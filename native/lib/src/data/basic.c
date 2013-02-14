#include "../internal.h"

static const char *mr_vector_type_name(mr_type_t self, mr_object_t object) { return "VECTOR"; }
static void mr_vector_type_enumerate_ref(mr_type_t self, mr_object_t object, void(*touch)(void *, mr_object_t), void *priv) {
    mr_vector_t vector = object;
    mr_uintptr_t i;
    for (i = 0; i < vector->length; ++ i)
        touch(priv, vector->entry[i]);
}
static void mr_vector_type_free(mr_type_t self, mr_object_t object) {
    mr_vector_t vector = object;
    if (vector->length)
        MR_FREE(vector->entry);
}
static mr_type_s __mr_vector_type = {
    .name          = mr_vector_type_name,
    .enumerate_ref = mr_vector_type_enumerate_ref,
    .free          = mr_vector_type_free,
};

static const char *mr_pair_type_name(mr_type_t self, mr_object_t object) { return "PAIR"; }
static void mr_pair_type_enumerate_ref(mr_type_t self, mr_object_t object, void(*touch)(void *, mr_object_t), void *priv) {
    mr_pair_t pair = object;
    touch(priv, pair->car);
    touch(priv, pair->cdr);
}
static void mr_pair_type_free(mr_type_t self, mr_object_t object) {
}
static mr_type_s __mr_pair_type = {
    .name          = mr_pair_type_name,
    .enumerate_ref = mr_pair_type_enumerate_ref,
    .free          = mr_pair_type_free,
};

static const char *mr_string_type_name(mr_type_t self, mr_object_t object) { return "STRING"; }
static void mr_string_type_enumerate_ref(mr_type_t self, mr_object_t object, void(*touch)(void *, mr_object_t), void *priv) {
}
static void mr_string_type_free(mr_type_t self, mr_object_t object) {
    mr_string_t string = object;
    if (string->length)
        MR_FREE(string->entry);
}
static mr_type_s __mr_string_type = {
    .name          = mr_string_type_name,
    .enumerate_ref = mr_string_type_enumerate_ref,
    .free          = mr_string_type_free,
};

mr_type_t mr_vector_type = &__mr_vector_type;
mr_type_t mr_pair_type   = &__mr_pair_type;
mr_type_t mr_string_type = &__mr_string_type;

mr_vector_t
mr_vector_allocate(mr_heap_t heap, mr_uintptr_t size)
{
    mr_vector_t v = MR_OBJECT_NEW(heap, mr_vector_s);
    if (v == NULL) return v;
    v->length = size;
    v->entry  = MR_MALLOC(size * sizeof(mr_object_t));
    if (v->entry == NULL)
    {
        mr_object_remove_extern_ref(v);
        return NULL;
    }
    MR_OBJECT_TYPE_INIT(v, mr_vector_type);
    return v;
}

mr_pair_t
mr_pair_create(mr_heap_t heap, mr_object_t car, mr_object_t cdr)
{
    mr_pair_t p = MR_OBJECT_NEW(heap, mr_pair_s);
    if (p == NULL) return p;
    p->car = car;
    p->cdr = cdr;
    MR_OBJECT_TYPE_INIT(p, mr_pair_type);
    return p;
}

mr_string_t
mr_string_with_pre_buffer(mr_heap_t heap, void *entry, mr_uintptr_t length)
{
    mr_string_t s = MR_OBJECT_NEW(heap, mr_string_s);
    if (s == NULL) return s;
    s->length = length;
    s->entry  = entry;
    MR_OBJECT_TYPE_INIT(s, mr_string_type);
    return s;
}
