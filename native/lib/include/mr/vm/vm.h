#ifndef __MR_VM_H__
#define __MR_VM_H__

#include "base.h"

#define MR_OBJECT_VM_EXCEPTION_REGS_OUT_OF_RANGE MR_SYMBOL_BOX(MR_SYMBOL_VM_OFFSET + 0)
#define MR_OBJECT_VM_EXCEPTION_APPLY_ARGC_ERROR  MR_SYMBOL_BOX(MR_SYMBOL_VM_OFFSET + 1)
#define MR_OBJECT_VM_EXCEPTION_APPLY_BAD_ENTRY   MR_SYMBOL_BOX(MR_SYMBOL_VM_OFFSET + 2)
#define MR_OBJECT_VM_EXCEPTION_APPLY_NO_MEM      MR_SYMBOL_BOX(MR_SYMBOL_VM_OFFSET + 3)
#define MR_OBJECT_VM_EXCEPTION_APPLY_UNSOLVED    MR_SYMBOL_BOX(MR_SYMBOL_VM_OFFSET + 4)
#define MR_OBJECT_VM_EXCEPTION_BAD_INSTRUCTION   MR_SYMBOL_BOX(MR_SYMBOL_VM_OFFSET + 5)

struct mr_trace_s;

typedef struct mr_vm_s      *mr_vm_t;
typedef struct mr_frame_s   *mr_frame_t;

typedef struct mr_frame_s
{
    mr_vm_uword_t       ret_dst;
    struct mr_vector_s *regs;
    mr_frame_t          frame;
    struct mr_vector_s *binding;
    struct mr_trace_s  *trace;
    mr_vm_uword_t       pc;
} mr_frame_s;

typedef struct mr_vm_s
{
    struct mr_heap_s   *heap;
    struct mr_vector_s *regs;
    mr_frame_t          frame;
    struct mr_vector_s *binding;
    struct mr_trace_s  *trace;
    mr_vm_uword_t       pc;
    void               *priv;
} mr_vm_s;

extern struct mr_type_s *mr_frame_type;
extern struct mr_type_s *mr_vm_type;

mr_frame_t  mr_frame_create_from_vm(mr_vm_t vm, mr_vm_uword_t ret_dst);
mr_vm_t     mr_vm_create(struct mr_heap_s *heap);
int         mr_vm_init_apply(mr_vm_t vm, struct mr_closure_s *closure, unsigned int argc, void **args);
mr_object_t mr_vm_run(mr_vm_t vm, mr_object_t external_slot);

#endif
