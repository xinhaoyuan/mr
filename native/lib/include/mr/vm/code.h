#ifndef __MR_CODE_H__
#define __MR_CODE_H__

#include "base.h"

#define INS_CODE_CONSTANT       0
#define INS_CODE_LOAD           1
#define INS_CODE_STORE          2
#define INS_CODE_BRANCH         3
#define INS_CODE_JUMP           4
#define INS_CODE_LAMBDA         5
#define INS_CODE_APPLY_PREPARE  6
#define INS_CODE_APPLY_PUSH_ARG 7
#define INS_CODE_APPLY          8
#define INS_CODE_APPLY_TAIL     9
#define INS_CODE_RET            10

typedef struct mr_lambda_info_s *mr_lambda_info_t;
typedef struct mr_trace_s       *mr_trace_t;
typedef struct mr_instruction_s *mr_instruction_t;

typedef struct mr_lambda_info_s
{
    mr_vm_uword_t argc;
    mr_vm_uword_t regs_size;
} mr_lambda_info_s;
    
typedef struct mr_trace_s
{
    struct mr_vector_s      *ref_vector;
    mr_lambda_info_t         lambda_info;
    mr_vm_uword_t            reg_max;
    mr_vm_uword_t            length;
    struct mr_instruction_s *entry;
} mr_trace_s;

typedef struct mr_instruction_s
{
    mr_vm_uword_t ins_code;    
    union
    {
        struct
        {
            mr_vm_uword_t id;
            mr_vm_uword_t dst;
        } constant;

        struct
        {
            mr_vm_uword_t level;
            mr_vm_uword_t offset;
            mr_vm_uword_t dst;
        } load;

        struct
        {
            mr_vm_uword_t level;
            mr_vm_uword_t offset;
            mr_vm_uword_t src;
        } store;

        struct
        {
            mr_trace_t    then_trace;
            mr_trace_t    else_trace;
            mr_vm_uword_t src;
        } branch;

        struct
        {
            mr_trace_t target_trace;
        } jump;

        struct
        {
            mr_trace_t    entry_trace;
            mr_vm_uword_t dst;
        } lambda;

        struct
        {
            mr_vm_uword_t src;
            mr_vm_uword_t length;
        } apply_prepare;

        struct
        {
            mr_vm_uword_t src;
        } apply_push_arg;

        struct
        {
            mr_vm_uword_t dst;
        } apply;

        struct
        {
            mr_vm_uword_t src;
        } ret;
    };
} mr_instruction_s;

typedef struct mr_closure_s *mr_closure_t;
typedef struct mr_closure_s
{
    mr_vm_uword_t       argc;
    mr_vm_uword_t       regs_size;
    mr_trace_t          entry_trace;
    struct mr_vector_s *parent_binding;
} mr_closure_s;

#include <mr/io/char_stream.h>

struct mr_vm_s;

typedef void*(*constant_parser_f)(struct mr_vm_s *, mr_io_char_stream_in_f, void *);
mr_closure_t mr_closure_parse_bin(struct mr_vm_s *vm, mr_io_char_stream_in_f in, constant_parser_f constant_parser, void *stream);
mr_closure_t mr_closure_create(struct mr_heap_s *heap, mr_trace_t entry_trace, struct mr_vector_s *parent_binding);

extern struct mr_type_s *mr_closure_type;
extern struct mr_type_s *mr_trace_type;

#endif
