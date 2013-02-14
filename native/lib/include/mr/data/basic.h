#ifndef __MR_DATA_BASIC_H__
#define __MR_DATA_BASIC_H__

#include "object.h"

typedef struct mr_vector_s *mr_vector_t;
typedef struct mr_pair_s   *mr_pair_t;
typedef struct mr_string_s *mr_string_t;

typedef struct mr_vector_s
{
    mr_uintptr_t length;
    mr_object_t *entry;
} mr_vector_s;

typedef struct mr_pair_s
{
    mr_object_t car, cdr;
} mr_pair_s;

typedef struct mr_string_s
{
    mr_uintptr_t   length;
    unsigned char *entry;
} mr_string_s;

extern mr_type_t mr_vector_type;
extern mr_type_t mr_pair_type;
extern mr_type_t mr_string_type;

mr_vector_t mr_vector_allocate(mr_heap_t heap, mr_uintptr_t size);
mr_pair_t   mr_pair_create(mr_heap_t heap, mr_object_t car, mr_object_t cdr);
mr_string_t mr_string_with_pre_buffer(mr_heap_t heap, void *entry, mr_uintptr_t length);

#endif
