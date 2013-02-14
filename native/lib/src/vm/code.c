#include "../internal.h"

static const char *mr_trace_type_name(mr_type_t self, mr_object_t object) { return "TRACE"; }
static void mr_trace_type_enumerate_ref(mr_type_t self, mr_object_t object, void(*touch)(void *, mr_object_t), void *priv) {
    mr_trace_t trace = object;
    touch(priv, trace->ref_vector);
}
static void mr_trace_type_free(mr_type_t self, mr_object_t object) {
    mr_trace_t trace = object;
    if (trace->lambda_info)
        MR_FREE(trace->lambda_info);
    if (trace->length)
    {
        MR_FREE(trace->entry);
    }
}
static mr_type_s __mr_trace_type = {
    .name          = mr_trace_type_name,
    .enumerate_ref = mr_trace_type_enumerate_ref,
    .free          = mr_trace_type_free,
};

static const char *mr_closure_type_name(mr_type_t self, mr_object_t object) { return "CLOSURE"; }
static void mr_closure_type_enumerate_ref(mr_type_t self, mr_object_t object, void(*touch)(void *, mr_object_t), void *priv) {
    mr_closure_t closure = object;
    touch(priv, closure->entry_trace);
    if (closure->parent_binding)
        touch(priv, closure->parent_binding);
}

static void mr_closure_type_free(mr_type_t self, mr_object_t object) { }
static mr_type_s __mr_closure_type = {
    .name          = mr_closure_type_name,
    .enumerate_ref = mr_closure_type_enumerate_ref,
    .free          = mr_closure_type_free,
};

mr_type_t mr_trace_type   = &__mr_trace_type;
mr_type_t mr_closure_type = &__mr_closure_type;

mr_closure_t
mr_closure_parse_bin(mr_vm_t vm, mr_io_char_stream_in_f in, constant_parser_f constant_parser, void *stream)
{
    mr_vm_uword_t num_trace, entry_id;
    mr_vm_uword_t i;
    mr_trace_t *traces = NULL;
    mr_closure_t closure = NULL;
    
    if (MR_IO_CS_READ_U(in, stream, num_trace))
        goto failed;
    if (MR_IO_CS_READ_U(in, stream, entry_id))
        goto failed;
    if (entry_id >= num_trace)
        goto failed;
    
    traces = MR_MALLOC(sizeof(mr_trace_t) * num_trace);
    if (traces == NULL) goto failed;
    memset(traces, 0, sizeof(mr_trace_t) * num_trace);

    closure = MR_OBJECT_NEW(vm->heap, mr_closure_s);
    if (closure == NULL) goto failed;
    
    /* Allocate trace objects */
    for (i = 0; i < num_trace; ++ i)
    {
        traces[i] = MR_OBJECT_NEW(vm->heap, mr_trace_s);
        if (traces[i] == NULL)
            goto failed;
    }

    for (i = 0; i < num_trace; ++ i)
    {
        mr_vm_uword_t
            has_lambda_info,
            ins_index,
            ref_cur,
            reg_max,
            in_apply_length,
            ins_count = 0,
            const_count = 0,
            ref_count = 0;
        mr_vector_t ref_vector = NULL;
        mr_instruction_t ins_entry = NULL;
        mr_lambda_info_t lambda_info = NULL;
        
        if (MR_IO_CS_READ_U(in, stream, has_lambda_info))
            goto trace_failed;

        if (has_lambda_info)
        {
            lambda_info = MR_MALLOC(sizeof(mr_lambda_info_s));
            if (lambda_info == NULL) goto trace_failed;

            if (MR_IO_CS_READ_U(in, stream, lambda_info->argc))
                goto trace_failed;

            if (MR_IO_CS_READ_U(in, stream, lambda_info->regs_size))
                goto trace_failed;
        }
        
        if (MR_IO_CS_READ_U(in, stream, ins_count))
            goto trace_failed;

        ins_entry = MR_MALLOC(sizeof(mr_instruction_s) * ins_count);
        if (ins_entry == NULL)
            goto trace_failed;

        reg_max = 0;
        ref_count = 0;
        in_apply_length = 0;
        for (ins_index = 0; ins_index < ins_count; ++ ins_index)
        {
            mr_vm_uword_t ins_code;
            if (MR_IO_CS_READ_U(in, stream, ins_code))
            {
                goto trace_failed;
            }

            mr_instruction_t ins = &ins_entry[ins_index];
            ins->ins_code = ins_code;
            switch (ins_code)
            {
            case INS_CODE_CONSTANT:
                if (in_apply_length) goto trace_failed;
                if (MR_IO_CS_READ_U(in, stream, ins->constant.id))
                    goto trace_failed;
                if (MR_IO_CS_READ_U(in, stream, ins->constant.dst))
                    goto trace_failed;
                if (ins->constant.dst > reg_max)
                    reg_max = ins->constant.dst;
                break;
                
            case INS_CODE_LOAD:
                if (in_apply_length) goto trace_failed;
                if (MR_IO_CS_READ_U(in, stream, ins->load.level))
                    goto trace_failed;
                if (MR_IO_CS_READ_U(in, stream, ins->load.offset))
                    goto trace_failed;
                if (MR_IO_CS_READ_U(in, stream, ins->load.dst))
                    goto trace_failed;
                if (ins->load.dst > reg_max)
                    reg_max = ins->load.dst;
                break;

            case INS_CODE_STORE:
                if (in_apply_length) goto trace_failed;
                if (MR_IO_CS_READ_U(in, stream, ins->store.level))
                    goto trace_failed;
                if (MR_IO_CS_READ_U(in, stream, ins->store.offset))
                    goto trace_failed;
                if (MR_IO_CS_READ_U(in, stream, ins->store.src))
                    goto trace_failed;
                if (ins->store.src > reg_max)
                    reg_max = ins->store.src;
                break;
                
            case INS_CODE_BRANCH:
            {
                if (in_apply_length) goto trace_failed;
                mr_vm_uword_t t, f;
                if (MR_IO_CS_READ_U(in, stream, t))
                    goto trace_failed;
                if (MR_IO_CS_READ_U(in, stream, f))
                    goto trace_failed;
                if (f >= num_trace || t >= num_trace)
                    goto trace_failed;
                ins->branch.then_trace = traces[t];
                ins->branch.else_trace = traces[f];
                if (MR_IO_CS_READ_U(in, stream, ins->branch.src))
                    goto trace_failed;
                ref_count += 2;
                if (ins->branch.src > reg_max)
                    reg_max = ins->branch.src;
                break;
            }

            case INS_CODE_JUMP:
            {
                if (in_apply_length) goto trace_failed;
                uint32_t target;
                if (MR_IO_CS_READ_U(in, stream, target)) goto trace_failed;
                if (target >= num_trace) goto trace_failed;
                ins->jump.target_trace = traces[target];
                ref_count += 1;
                break;
            }

            case INS_CODE_APPLY_PREPARE:
            {
                if (in_apply_length) goto trace_failed;
                if (MR_IO_CS_READ_U(in, stream, ins->apply_prepare.src))
                    goto trace_failed;
                if (ins->apply_prepare.src > reg_max)
                    reg_max = ins->apply_prepare.src;
                in_apply_length = 1;
                break;
            }

            case INS_CODE_APPLY_PUSH_ARG:
            {
                if (!in_apply_length) goto trace_failed;
                if (MR_IO_CS_READ_U(in, stream, ins->apply_push_arg.src))
                    goto trace_failed;
                if (ins->apply_push_arg.src > reg_max)
                    reg_max = ins->apply_push_arg.src;
                ++ in_apply_length;
                break;
            }

            case INS_CODE_APPLY:
            {
                if (!in_apply_length) goto trace_failed;
                if (MR_IO_CS_READ_U(in, stream, ins->apply.dst))
                    goto trace_failed;
                if (ins->apply.dst > reg_max)
                    reg_max = ins->apply.dst;
                ins_entry[ins_index - in_apply_length].apply_prepare.length = in_apply_length;
                in_apply_length = 0;
                break;
            }

            case INS_CODE_APPLY_TAIL:
            {
                if (!in_apply_length) goto trace_failed;
                ins_entry[ins_index - in_apply_length].apply_prepare.length = in_apply_length;
                in_apply_length = 0;
                break;
            }

            case INS_CODE_RET:
                if (in_apply_length) goto trace_failed;
                if (MR_IO_CS_READ_U(in, stream, ins->ret.src))
                    goto trace_failed;
                 if (ins->ret.src > reg_max)
                    reg_max = ins->ret.src;
                break;

            case INS_CODE_LAMBDA:
            {
                if (in_apply_length) goto trace_failed;
                uint32_t code;
                if (MR_IO_CS_READ_U(in, stream, code)) goto trace_failed;
                if (code >= num_trace) goto trace_failed;
                ins->lambda.entry_trace = traces[code];
                if (mr_io_cs_read_u(in, stream, &ins->lambda.dst, sizeof(ins->lambda.dst)))
                    goto trace_failed;
                ref_count += 1;
                break;
            }

            default:
                goto trace_failed;
            }
        }

        /* Can only ends with these transit instructions */
        if (ins_entry[ins_count - 1].ins_code != INS_CODE_BRANCH &&
            ins_entry[ins_count - 1].ins_code != INS_CODE_JUMP &&
            ins_entry[ins_count - 1].ins_code != INS_CODE_RET &&
            ins_entry[ins_count - 1].ins_code != INS_CODE_APPLY_TAIL)
            goto trace_failed;

        if (MR_IO_CS_READ_U(in, stream, const_count))
            goto trace_failed;

        if (const_count + ref_count)
        {
            ref_vector = mr_vector_allocate(vm->heap, const_count + ref_count);
            if (ref_vector == NULL) goto trace_failed;
            MR_MEMSET(ref_vector->entry, 0, sizeof(mr_object_t) * (const_count + ref_count));
        }

        /* Read constants */
        for (ref_cur = 0; ref_cur < const_count; ++ ref_cur)
        {
            ref_vector->entry[ref_cur] = constant_parser(vm, in, stream);
            if (ref_vector->entry[ref_cur] == NULL)
                goto trace_failed;
            else mr_object_remove_extern_ref(ref_vector->entry[ref_cur]);
        }

        for (ins_index = 0; ins_index < ins_count; ++ ins_index)
        {
            switch (ins_entry[ins_index].ins_code)
            {
            case INS_CODE_BRANCH:
                ref_vector->entry[ref_cur ++] = ins_entry[ins_index].branch.then_trace;
                ref_vector->entry[ref_cur ++] = ins_entry[ins_index].branch.else_trace;
                break;

            case INS_CODE_JUMP:
                ref_vector->entry[ref_cur ++] = ins_entry[ins_index].jump.target_trace;
                break;

            case INS_CODE_LAMBDA:
                ref_vector->entry[ref_cur ++] = ins_entry[ins_index].lambda.entry_trace;
                break;
            }
        }
        if (ref_cur != ref_count + const_count) goto trace_failed;
        
        traces[i]->ref_vector  = ref_vector;
        traces[i]->lambda_info = lambda_info;
        traces[i]->reg_max     = reg_max;
        traces[i]->length      = ins_count;
        traces[i]->entry       = ins_entry;
        MR_OBJECT_TYPE_INIT(traces[i], mr_trace_type);

        mr_object_remove_extern_ref(ref_vector);

        continue;

      trace_failed:
        if (ref_vector)
            mr_object_remove_extern_ref(ref_vector);
        if (lambda_info)
            MR_FREE(lambda_info);
        if (ins_entry)
            MR_FREE(ins_entry);
        goto failed;
    }

    if (traces[entry_id]->lambda_info == NULL) goto failed;

    closure->argc           = traces[entry_id]->lambda_info->argc;
    closure->regs_size      = traces[entry_id]->lambda_info->regs_size;
    closure->entry_trace    = traces[entry_id];
    closure->parent_binding = NULL;
    MR_OBJECT_TYPE_INIT(closure, mr_closure_type);

    for (i = 0; i < num_trace; ++ i)
        mr_object_remove_extern_ref(traces[i]);
    fprintf(stderr, "parse bin ok\n");
    return closure;
        
  failed:
    fprintf(stderr, "parse bin failed\n");
    if (closure != NULL)
        mr_object_remove_extern_ref(closure);
    if (traces != NULL)
    {
        for (i = 0; i < num_trace; ++ i)
            if (traces[i] != NULL)
                mr_object_remove_extern_ref(traces[i]);
        MR_FREE(traces);
    }
    return NULL;
}

mr_closure_t
mr_closure_create(mr_heap_t heap, mr_trace_t entry_trace, mr_vector_t parent_binding)
{
    if (entry_trace->lambda_info == NULL) return NULL;
    mr_closure_t closure = MR_OBJECT_NEW(heap, mr_closure_s);
    if (closure == NULL) return NULL;
    closure->argc           = entry_trace->lambda_info->argc;
    closure->regs_size      = entry_trace->lambda_info->regs_size;
    closure->entry_trace    = entry_trace;
    closure->parent_binding = parent_binding;
    MR_OBJECT_TYPE_INIT(closure, mr_closure_type);
    return closure;
}
