#include "../internal.h"

static const char *mr_vm_type_name(mr_type_t self, mr_object_t object) { return "VM"; }
static void mr_vm_type_enumerate_ref(mr_type_t self, mr_object_t object, void(*touch)(void *, mr_object_t), void *priv) {
    mr_vm_t vm = object;
    touch(priv, vm->regs);
    touch(priv, vm->frame);
    touch(priv, vm->binding);
    touch(priv, vm->trace);
}
static void mr_vm_type_free(mr_type_t self, mr_object_t object) {
}
static mr_type_s __mr_vm_type = {
    .name          = mr_vm_type_name,
    .enumerate_ref = mr_vm_type_enumerate_ref,
    .free          = mr_vm_type_free,
};

static const char *mr_frame_type_name(mr_type_t self, mr_object_t object) { return "FRAME"; }
static void mr_frame_type_enumerate_ref(mr_type_t self, mr_object_t object, void(*touch)(void *, mr_object_t), void *priv) {
    mr_frame_t frame = object;
    touch(priv, frame->regs);
    touch(priv, frame->frame);
    touch(priv, frame->binding);
    touch(priv, frame->trace);
}
static void mr_frame_type_free(mr_type_t self, mr_object_t object) {
}
static mr_type_s __mr_frame_type = {
    .name          = mr_frame_type_name,
    .enumerate_ref = mr_frame_type_enumerate_ref,
    .free          = mr_frame_type_free,
};

mr_type_t mr_frame_type = &__mr_frame_type;
mr_type_t mr_vm_type    = &__mr_vm_type;

mr_frame_t
mr_frame_create_from_vm(mr_vm_t vm, mr_vm_uword_t ret_dst)
{
    mr_frame_t frame = MR_OBJECT_NEW(vm->heap, mr_frame_s);
    if (frame == NULL) return NULL;
    frame->ret_dst = ret_dst;
    frame->regs    = vm->regs;
    frame->frame   = vm->frame;
    frame->binding = vm->binding;
    frame->trace   = vm->trace;
    frame->pc      = vm->pc;
    MR_OBJECT_TYPE_INIT(frame, mr_frame_type);
    return frame;
}

mr_vm_t
mr_vm_create(mr_heap_t heap)
{
    mr_vm_t vm = MR_OBJECT_NEW(heap, mr_vm_s);
    if (vm == NULL) return NULL;
    vm->heap    = heap;
    vm->regs    = NULL;
    vm->frame   = NULL;
    vm->binding = NULL;
    vm->trace   = NULL;
    vm->pc      = 0;
    MR_OBJECT_TYPE_INIT(vm, mr_vm_type);
    return vm;
}

int
mr_vm_init_apply(mr_vm_t vm, mr_closure_t closure, unsigned int argc, void **args)
{
    if (closure->argc != argc)
        return -1;
    vm->regs      = mr_vector_allocate(vm->heap, sizeof(mr_object_t) * closure->regs_size);
    if (vm->regs == NULL) return -1;
    mr_object_remove_extern_ref(vm->regs);
    vm->frame     = NULL;
    vm->binding   = mr_vector_allocate(vm->heap, sizeof(mr_object_t) * (closure->argc + 1));
    if (vm->binding == NULL)
    {
        vm->regs = NULL;
        return -1;
    }
    mr_object_remove_extern_ref(vm->binding);
    vm->trace     = closure->entry_trace;
    vm->pc        = 0;
    MR_MEMSET(vm->regs->entry, 0, sizeof(mr_object_t) * closure->regs_size);
    MR_MEMCPY(vm->binding->entry + 1, args, closure->argc);
    vm->binding->entry[0] = NULL;
    return 0;
}

int
mr_vm_apply_extract(mr_vm_t vm, mr_vm_uword_t *argc, void ***args)
{
    mr_instruction_t ins = &vm->trace->entry[vm->pc];
    if (ins->ins_code == INS_CODE_APPLY_PREPARE)
    {
        *argc = ins->apply_prepare.length;
        mr_vm_uword_t i;
        mr_object_t *entry = MR_MALLOC(*argc * sizeof(mr_object_t));
        if (entry == NULL) return -1;
        entry[0] = vm->regs->entry[ins->apply_prepare.src];
        for (i = 1; i < *argc; ++ i)
            entry[i] = vm->regs->entry[ins[i].apply_push_arg.src];
        vm->pc += *argc;
        // fprintf(stderr, "pc = %u after extract\n", vm->pc);
        *args = entry;
        return 0;
    }
    else return -1;
}

mr_object_t
mr_vm_run(mr_vm_t vm, mr_object_t external_slot)
{
    register mr_instruction_t ins;
    mr_object_t *regs;
    mr_instruction_t ins_bound;
    unsigned int     regs_size;
    mr_object_t      result = NULL;

  start:
    
    ins       = &vm->trace->entry[vm->pc];
    ins_bound = &vm->trace->entry[vm->trace->length];
    regs_size = vm->regs->length;
    regs      = vm->regs->entry;

    while (ins < ins_bound)
    {
        // fprintf(stderr, "ins %d at %p:%u\n", ins->ins_code, vm->trace->entry, ins - vm->trace->entry);
        switch (ins->ins_code)
        {
        case INS_CODE_CONSTANT:
            regs[ins->constant.dst] = vm->trace->ref_vector->entry[ins->constant.id];
            ++ ins;
            break;

        case INS_CODE_LOAD:
        {
            mr_vm_uword_t i = ins->load.level;
            mr_vector_t binding = vm->binding;
            while (i > 0 && binding)
            {
                -- i;
                binding = binding->entry[0];
            }
            if (i == 0)
                regs[ins->load.dst] = binding->entry[ins->load.offset + 1];
            ++ ins;
            break;
        }

        case INS_CODE_STORE:
        {
            mr_vm_uword_t i = ins->store.level;
            mr_vector_t binding = vm->binding;
            while (i > 0 && binding)
            {
                -- i;
                binding = binding->entry[0];
            }
            if (i == 0)
                binding->entry[ins->store.offset + 1] = regs[ins->store.src];
            ++ ins;
            break;
        }

        case INS_CODE_BRANCH:
        {
            mr_trace_t target;
            if (regs[ins->branch.src] != MR_OBJECT_NULL &&
                regs[ins->branch.src] != MR_OBJECT_FALSE)
                target = ins->branch.then_trace;
            else
                target = ins->branch.else_trace;
            if (target->reg_max >= regs_size)
            {
                result = MR_SYMBOL_VM_EXCEPTION_REGS_OUT_OF_RANGE;
                goto vmexit;
            }
            vm->trace = target;
            vm->pc    = 0;
            ins       = &vm->trace->entry[vm->pc];
            ins_bound = &vm->trace->entry[vm->trace->length];
            break;
        }

        case INS_CODE_JUMP:
            if (ins->jump.target_trace->reg_max >= regs_size)
            {
                result = MR_SYMBOL_VM_EXCEPTION_REGS_OUT_OF_RANGE;
                goto vmexit;
            }
            vm->trace = ins->jump.target_trace;
            vm->pc    = 0;
            ins       = &vm->trace->entry[vm->pc];
            ins_bound = &vm->trace->entry[vm->trace->length];
            break;

        case INS_CODE_LAMBDA:
        {
            mr_closure_t closure = mr_closure_create(vm->heap, ins->lambda.entry_trace, vm->binding);
            if (closure)
            {
                regs[ins->lambda.dst] = closure;
                mr_object_remove_extern_ref(closure);
            }
            ++ ins;
            break;
        }

        case INS_CODE_APPLY_PREPARE:
        {
            mr_object_t f = regs[ins->apply_prepare.src];
            if (MR_OBJECT_TYPE_EQ(f, mr_closure_type))
            {
                mr_closure_t c = f;
                if (c->argc != ins->apply_prepare.length - 1)
                {
                    /* exception - argc not match */
                    result = MR_SYMBOL_VM_EXCEPTION_APPLY_ARGC_ERROR;
                    goto vmexit;
                }
                if (c->entry_trace->lambda_info == NULL)
                {
                    /* exception - invalid lambda info */
                    result = MR_SYMBOL_VM_EXCEPTION_APPLY_BAD_ENTRY;
                    goto vmexit;
                }
                mr_vector_t n_regs = mr_vector_allocate(vm->heap, c->entry_trace->lambda_info->regs_size);
                if (n_regs == NULL)
                {
                    /* exception - cannot allocate regs */
                    result = MR_SYMBOL_VM_EXCEPTION_APPLY_NO_MEM;
                    goto vmexit;
                }
                mr_vector_t n_binding = mr_vector_allocate(vm->heap, c->argc + 1);
                if (n_binding == NULL)
                {
                    mr_object_remove_extern_ref(n_regs);
                    /* exception - cannot allcoate binding vec */
                    result = MR_SYMBOL_VM_EXCEPTION_APPLY_NO_MEM;
                    goto vmexit;
                }
                mr_instruction_t ins_end = (ins + ins->apply_prepare.length);
                if (ins_end->ins_code == INS_CODE_APPLY)
                {
                    vm->pc = ins_end - vm->trace->entry + 1;
                    /* push frame */
                    mr_frame_t frame = mr_frame_create_from_vm(vm, ins_end->apply.dst);
                    if (frame == NULL)
                    {
                        mr_object_remove_extern_ref(n_regs);
                        mr_object_remove_extern_ref(n_binding);
                        /* exception - cannot create returning frame */
                        result = MR_SYMBOL_VM_EXCEPTION_APPLY_NO_MEM;
                        goto vmexit;
                    }
                    vm->frame = frame;
                    mr_object_remove_extern_ref(frame);
                }
                /* fill args */
                n_binding->entry[0] = c->parent_binding;
                for (ins_end = ins + 1; ins_end < (ins + ins->apply_prepare.length); ++ ins_end)
                    n_binding->entry[ins_end - ins] = regs[ins_end->apply_push_arg.src];
                vm->regs    = n_regs;
                vm->binding = n_binding;
                vm->trace   = c->entry_trace;
                vm->pc      = 0;
                mr_object_remove_extern_ref(n_regs);
                mr_object_remove_extern_ref(n_binding);
                goto start;
            }
            else
            {
                result = MR_SYMBOL_VM_EXCEPTION_APPLY_UNSOLVED;
                goto vmexit;
            }
            break;
        }

        case INS_CODE_APPLY_PUSH_ARG:
            /* handled in apply_prepare */
            break;

        case INS_CODE_APPLY:
        {
            /* reach this code when the apply is solved externally */
            regs[ins->apply.dst] = external_slot;
            mr_object_remove_extern_ref(external_slot);
            ++ ins;
            break;
        }

        case INS_CODE_APPLY_TAIL:
        {
            /* reach this code when the apply is solved externally */
            /* same action as ret */
            mr_frame_t frame = vm->frame;
            if (frame == NULL)
            {
                /* Top level return */
                result = external_slot;
                goto vmexit;
            }
            else
            {
                /* Write the return value */
                frame->regs->entry[frame->ret_dst] = external_slot;
                mr_object_remove_extern_ref(external_slot);
                vm->regs    = frame->regs;
                vm->frame   = frame->frame;
                vm->binding = frame->binding;
                vm->trace   = frame->trace;
                vm->pc      = frame->pc;
                goto start;
            }
            break;
        }
            
        case INS_CODE_RET:
        {
            mr_frame_t frame = vm->frame;
            mr_object_t ret = regs[ins->ret.src];
            if (frame == NULL)
            {
                /* Top level return */
                mr_object_add_extern_ref(ret);
                result = ret;
                goto vmexit;
            }
            else
            {
                /* Write the return value */
                frame->regs->entry[frame->ret_dst] = ret;
                vm->regs    = frame->regs;
                vm->frame   = frame->frame;
                vm->binding = frame->binding;
                vm->trace   = frame->trace;
                vm->pc      = frame->pc;
                goto start;
            }
            break;
        }

        default:
            result = MR_SYMBOL_VM_EXCEPTION_BAD_INSTRUCTION;
            goto vmexit;
        }
    }

  vmexit:
    vm->pc = ins - vm->trace->entry;
    return result;
}
