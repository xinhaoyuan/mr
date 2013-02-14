#include "../internal.h"

#define HAS_GC_HEADER(object) ((object) && ((mr_uintptr_t)(object) & MR_TYPE_MASK) == 0)

#define GC_STATUS_UNTOUCH    0
#define GC_STATUS_TOUCHED    1
#define GC_STATUS_HOST       2

#define GC_THRESHOLD_INIT    4192
#define GC_QUEUE_LENGTH_INIT 256

static const char *dummy_type_name(mr_type_t self, mr_object_t object) { return "DUMMY"; }
static void dummy_type_enumerate_ref(mr_type_t self, mr_object_t object, void(*touch)(void *, mr_object_t), void *priv) { }
static void dummy_type_free(mr_type_t self, mr_object_t object) { }
static mr_type_s dummy_type = {
    .name          = dummy_type_name,
    .enumerate_ref = dummy_type_enumerate_ref,
    .free          = dummy_type_free,
};

static int do_gc(mr_heap_t heap);

mr_object_t 
mr_object_new_by_size(mr_heap_t heap, mr_uintptr_t size)
{
    if (heap && heap->object_count >= heap->gc_threshold)
    {
        if (do_gc(heap) != 0) return NULL;
    }

    mr_gc_header_t gc = (mr_gc_header_t)MR_MALLOC(sizeof(mr_gc_header_s) + size);
    if (gc == NULL) return NULL;

    gc->type          = &dummy_type;
    gc->gc_status     = heap ? GC_STATUS_UNTOUCH : GC_STATUS_HOST;
    gc->extern_ref    = 1;

    if (heap)
    {
        heap->tracker[heap->object_count] = MR_TO_OBJECT(gc);
        ++ heap->object_count;
    }
    
    return MR_TO_OBJECT(gc);
}

void
mr_object_host_free(mr_object_t object)
{
    if (!HAS_GC_HEADER(object)) return;
    mr_gc_header_t gc = MR_TO_GC_HEADER(object);
    
    if (gc->gc_status == GC_STATUS_HOST)
    {
        gc->type->free(gc->type, object);
        MR_FREE(gc);
    }
}

int
mr_object_add_extern_ref(mr_object_t object)
{
    if (!HAS_GC_HEADER(object)) return -1;
    mr_gc_header_t gc = MR_TO_GC_HEADER(object);
    
    if (gc->extern_ref < MR_EXTERN_REF_MAX)
    {
        ++ gc->extern_ref;
        return 0;
    }
    else return -1;
}

int
mr_object_remove_extern_ref(mr_object_t object)
{
    if (!HAS_GC_HEADER(object)) return -1;
    mr_gc_header_t gc = MR_TO_GC_HEADER(object);
    if (gc->extern_ref > 0)
    {
        -- gc->extern_ref;
        return 0;
    }
    else return -1;
}

typedef struct exqueue_s
{
    unsigned int alloc;
    unsigned int head;
    unsigned int tail;

    mr_object_t *queue;
} exqueue_s;

static void
exqueue_enqueue(exqueue_s *q, mr_object_t object)
{
    if (!HAS_GC_HEADER(object)) return;
    mr_gc_header_t gc = MR_TO_GC_HEADER(object);

    // fprintf(stderr, "touch %s %d\n", gc->type->name(gc->type, object), gc->gc_status);
    
    if (q->queue && gc->gc_status == GC_STATUS_UNTOUCH)
    {
        gc->gc_status = GC_STATUS_TOUCHED;
        
        q->queue[q->head ++] = object;
        q->head %= q->alloc;
        if (q->head == q->tail)
        {
            void *new = MR_REALLOC(q->queue, sizeof(mr_object_t) * (q->alloc << 1));
            if (new == NULL)
            {
                free(q->queue);
                q->queue = NULL;
            }
            else q->queue = (mr_object_t *)new;
            
            MR_MEMCPY(q->queue + q->alloc, q->queue, sizeof(mr_object_t) * q->head);
            q->head += q->alloc;
            q->alloc <<= 1;
        }
    }
}

static int
do_gc(mr_heap_t heap)
{
    exqueue_s q;
    q.alloc = GC_QUEUE_LENGTH_INIT;
    q.head  = 0;
    q.tail  = 0;
    q.queue = (mr_object_t *)MR_MALLOC(sizeof(mr_object_t) * q.alloc);

    mr_uintptr_t i;
    mr_gc_header_t gc;
    
    for (i = 0; i < heap->object_count; ++ i)
    {
        gc = MR_TO_GC_HEADER(heap->tracker[i]);
        gc->gc_status = GC_STATUS_UNTOUCH;
        if (gc->extern_ref)
            exqueue_enqueue(&q, heap->tracker[i]);
    }

    while (q.head != q.tail)
    {
        mr_object_t now = q.queue[q.tail ++];
        gc = MR_TO_GC_HEADER(now);
        q.tail %= q.alloc;

        // fprintf(stderr, "enum ref %s\n", gc->type->name(gc->type, now));
        gc->type->enumerate_ref(gc->type, now, (void(*)(void *,mr_object_t))exqueue_enqueue, &q);
    }

    if (q.queue == NULL) return -1;
    MR_FREE(q.queue);

    mr_uintptr_t count_new = 0;
    for (i = 0; i < heap->object_count; ++ i)
    {
        gc = MR_TO_GC_HEADER(heap->tracker[i]);
        if (gc->gc_status == GC_STATUS_TOUCHED)
            heap->tracker[count_new ++] = heap->tracker[i];
        else if (gc->gc_status == GC_STATUS_UNTOUCH)
        {
            // fprintf(stderr, "free %s\n", gc->type->name(gc->type, heap->tracker[i]));
            gc->type->free(gc->type, heap->tracker[i]);
            MR_FREE(gc);
        }
    }

    mr_uintptr_t threshold_new;
    
    /* Naive calcuation, should use binary instructions if possible */
    /* Should not overflow because the object count should be much
     * smaller than MAX_INTPTR */
    threshold_new = 1;
    while (threshold_new < count_new * 2)
        threshold_new = threshold_new << 1;
    if (threshold_new < GC_THRESHOLD_INIT)
        threshold_new = GC_THRESHOLD_INIT;

    void *tracker_new = MR_REALLOC(heap->tracker, sizeof(mr_object_t) * threshold_new);
    if (tracker_new)
    {
        heap->tracker = tracker_new;
        heap->gc_threshold = threshold_new;
    }
    heap->object_count = count_new;
    
    return 0;
}

mr_heap_t 
mr_heap_new(void)
{
    mr_heap_t heap;
    
    heap = (mr_heap_t)MR_MALLOC(sizeof(mr_heap_s));
    if (heap == NULL) return NULL;
    
    heap->object_count = 0;
    heap->gc_threshold = GC_THRESHOLD_INIT;
    heap->error_flag   = 0;
    
    if ((heap->tracker = (mr_object_t *)MR_MALLOC(sizeof(mr_object_t) * heap->gc_threshold)) == NULL)
    {
        MR_FREE(heap);
        return NULL;
    }
    
    return heap;
}

void
mr_heap_free(mr_heap_t heap)
{
    mr_uintptr_t i;
    mr_gc_header_t gc;
    for (i = 0; i < heap->object_count; ++ i)
    {
        gc = MR_TO_GC_HEADER(heap->tracker[i]);
        /* Currently we free all objects including those protected */
        gc->type->free(gc->type, heap->tracker[i]);
        MR_FREE(gc);
    }

    MR_FREE(heap->tracker);
    MR_FREE(heap);
}
