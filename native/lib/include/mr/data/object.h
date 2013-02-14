#ifndef __MR_DATA_OBEJCT_H__
#define __MR_DATA_OBEJCT_H__

#include "../config.h"

typedef void *mr_object_t;

/* Include the GC-specified part in object representation */
#ifndef __INSIDE_MR_DATA_OBJECT_H__
#define __INSIDE_MR_DATA_OBJECT_H__
#include "gc.h"
#undef __INSIDE_MR_DATA_OBJECT_H__
#else
#error macro __INSIDE_MR_DATA_OBJECT_H__ used outside ``object.h''?
#endif

#ifndef MR_OBJECT_TYPE_GET
#define MR_OBJECT_TYPE_GET(object) (mr_object_type_get(object))
#endif

#ifndef MR_OBJECT_TYPE_EQ
#define MR_OBJECT_SIMPLE_TYPE_EQ(object) (mr_object_simple_type_eq(object))
#endif

#ifndef MR_OBJECT_TYPE_INIT
#define MR_OBJECT_TYPE_INIT(object, type) do { mr_object_type_init(object, type); } while (0)
#endif

#ifndef MR_INT_BOX
#define MR_INT_BOX(num) (mr_int_box(num))
#endif

#ifndef MR_INT_UNBOX
#define MR_INT_UNBOX(object) (mr_int_unbox(object))
#endif

#ifndef MR_SYMBOL_BOX
#define MR_SYMBOL_BOX(num) (mr_symbol_box(num))
#endif

#ifndef MR_SYMBOL_UNBOX
#define MR_SYMBOL_UNBOX(object) (mr_symbol_unbox(object))
#endif

mr_heap_t mr_heap_new(void);
void      mr_heap_free(mr_heap_t heap);

/* heap == NULL means the object newed is managed by host code */
mr_object_t mr_object_new_by_size(mr_heap_t heap, mr_uintptr_t size);
/* and you could only free host objects */
void        mr_object_host_free(mr_object_t object);

#define MR_OBJECT_NEW(heap, type_layout) ((type_layout *)(mr_object_new_by_size(heap, sizeof(type_layout))))

int mr_object_add_extern_ref(mr_object_t object);
int mr_object_remove_extern_ref(mr_object_t object);

#endif
