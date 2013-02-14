#ifndef __MR_DATA_GC_H__
#define __MR_DATA_GC_H__

#ifndef __INSIDE_MR_DATA_OBJECT_H__
#error  ONLY INCLUDE THIS FILE IN ``object.h''
#endif

typedef struct mr_slot_s      *mr_slot_t;
typedef struct mr_heap_s      *mr_heap_t;
typedef struct mr_gc_header_s *mr_gc_header_t;
typedef struct mr_type_s      *mr_type_t;

#define MR_ITYPE_OBJECT    ((mr_type_t)0x00)
#define MR_ITYPE_INT       ((mr_type_t)0x01)
#define MR_ITYPE_SYMBOL    ((mr_type_t)0x02)
#define MR_IS_ITYPE(type)  ((mr_uintptr_t)type < 0x8)

/* SEE assumes that the malloc func is aligned to 8, thus leaving last
 * 3 bits of a pointer free for atomic data type masking */

/* The mask pattern is like this: */
/* XX1   -- Boxed integer         */
/* 010   -- Internal symbol       */
/* Other -- Undefined             */
#define MR_TYPE_MASK         0x7
#define MR_IS_INT(obj)       ((mr_uintptr_t)(obj) & 1)
#define MR_IS_SYMBOL(obj)    (((mr_uintptr_t)(obj) & 0x7) == MR_ITYPE_SYMBOL)
#define MR_INT_BOX(num)      ((mr_object_t)(((mr_uintptr_t)(num) << 1) | 1))
#define MR_INT_UNBOX(obj)    ((mr_intptr_t)(obj) >> 1)
#define MR_SYMBOL_BOX(id)    ((mr_object_t)(((mr_uintptr_t)(id) << 3) | (mr_uintptr_t)MR_ITYPE_SYMBOL))
#define MR_SYMBOL_UNBOX(obj) ((mr_uintptr_t)(obj) >> 3)

#define MR_SYMBOL_ID_NULL             0
#define MR_SYMBOL_ID_TRUE             1
#define MR_SYMBOL_ID_FALSE            2
#define MR_SYMBOL_VM_OFFSET           128
#define MR_SYMBOL_USER_OFFSET         256

#define MR_OBJECT_NULL                MR_SYMBOL_BOX(MR_SYMBOL_ID_NULL)
#define MR_OBJECT_TRUE                MR_SYMBOL_BOX(MR_SYMBOL_ID_TRUE)
#define MR_OBJECT_FALSE               MR_SYMBOL_BOX(MR_SYMBOL_ID_FALSE)

typedef struct mr_heap_s
{
    mr_uintptr_t    object_count;
    mr_uintptr_t    gc_threshold;
    
    mr_object_t  *tracker;
    int           error_flag;
} mr_heap_s;

struct mr_gc_header_s
{
    mr_type_t      type;
    unsigned short gc_status;
    unsigned short extern_ref;
} __attribute__((aligned(MR_PTR_SIZE)));
typedef struct mr_gc_header_s mr_gc_header_s;

#define MR_EXTERN_REF_MAX 0xffff

typedef struct mr_type_s
{
    const char*(*name)(mr_type_t self, mr_object_t object);
    void(*enumerate_ref)(mr_type_t self, mr_object_t object, void(*touch)(void *priv, mr_object_t data), void *priv);
    void(*free)(mr_type_t self, mr_object_t object);
} mr_type_s;

#define MR_TO_GC_HEADER(object)  ((mr_gc_header_t)(object) - 1)
#define MR_TO_OBJECT(gc)         ((mr_object_t)((mr_gc_header_t)(gc) + 1))

#define MR_OBJECT_TYPE_GET(__object) ({                                 \
            mr_object_t object = (__object);                            \
            mr_type_t result = (mr_type_t)((mr_uintptr_t)object & MR_TYPE_MASK); \
            if (result == NULL && object)                               \
                result = MR_TO_GC_HEADER(object)->type;                 \
            result;                                                     \
        })

#define MR_OBJECT_TYPE_EQ(__object, __type) ({                          \
            mr_object_t object = (__object);                            \
            mr_type_t type = (__type);                                  \
            mr_type_t dtype = (mr_type_t)((mr_uintptr_t)object & MR_TYPE_MASK); \
            if (!MR_IS_ITYPE(type))                                     \
                if (dtype == NULL && object)                            \
                    dtype = MR_TO_GC_HEADER(object)->type;              \
            dtype == type;                                              \
        })

#define MR_OBJECT_TYPE_INIT(__object, __type) do {  \
        MR_TO_GC_HEADER(__object)->type = (__type); \
    } while (0);

mr_object_t mr_object_new_by_size(mr_heap_t heap, mr_uintptr_t size);


#endif
