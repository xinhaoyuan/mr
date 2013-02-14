extern "C" {
    
#include <mr/data/object.h>
#include <mr/data/basic.h>
#include <mr/vm/code.h>
#include <mr/vm/vm.h>
#include <mr/io/token_parser.h>
#include "simple_stream.h"
#include <unistd.h>
}

#include <map>
#include <string>

using namespace std;

mr_object_t
constant_parser(mr_vm_t vm, mr_io_char_stream_in_f in, void *stream)
{
    mr_vm_uword_t i, desc_size;
    if (MR_IO_CS_READ_U(in, stream, desc_size))
        return NULL;
    neststream_s ns;
    neststream_open(&ns, in, stream, desc_size);
    fprintf(stderr, "parse for %u bytes\n", desc_size);
    int token_type;
    char *token_buf;
    unsigned int token_len;
    token_type = mr_io_cs_parse_token((mr_io_char_stream_in_f)neststream_in, &ns, &token_buf, &token_len);
    mr_object_t r = NULL;
    switch (token_type)
    {
    case MR_TOKEN_SYMBOL:
    {
        int is_num = 1;
        mr_vm_uword_t num_buf = 0;
        for (i = 0; i < token_len; ++ i)
        {
            if (token_buf[i] < '0' || token_buf[i] > '9')
            {
                is_num = 0;
                break;
            }
            num_buf = num_buf * 10 + (token_buf[i] - '0');
        }
        if (is_num)
        {
            r = MR_INT_BOX(num_buf);
            MR_FREE(token_buf);
        }
        else
        {
            string sym(token_buf, token_len);
            map<string, mr_object_t> *dict = (map<string, mr_object_t> *)vm->priv;
            map<string, mr_object_t>::iterator it = dict->find(sym);
            if (it != dict->end())
            {
                r = it->second;
                MR_FREE(token_buf);
            }
            else
            {
                (*dict)[sym] = r = mr_string_with_pre_buffer(vm->heap, token_buf, token_len);
            }
        }
        break;
    }
    case MR_TOKEN_STRING:
    {
        r = mr_string_with_pre_buffer(vm->heap, token_buf, token_len);
        break;
    }
    }
    fprintf(stderr, "read %u bytes\n", ns.pos);
    for (i = ns.pos; i < desc_size; ++ i)
        in(stream, MR_IO_CS_IN_ADVANCE);
    return r;
}

int
main(int argc, char **argv)
{
    fstream_s in;
    fstream_open(&in, stdin);
    mr_heap_t heap       = mr_heap_new();
    mr_vm_t   vm         = mr_vm_create(heap);
    map<string, mr_object_t> *dict = new map<string, mr_object_t>();
    vm->priv             = dict;
    mr_closure_t closure = mr_closure_parse_bin(
        vm,
        (mr_io_char_stream_in_f)fstream_in,
        constant_parser,
        &in);
    fstream_close(&in);
    mr_vm_init_apply(vm, closure, 0, NULL);
    mr_object_t r = NULL;
    
    mr_object_t print = (*dict)["print"];
    mr_object_t plus  = (*dict)["+"];
    mr_object_t lt    = (*dict)["<"];
    while (1)
    {
        r = mr_vm_run(vm, r);
        if (r == MR_SYMBOL_VM_EXCEPTION_APPLY_UNSOLVED)
        {
            mr_instruction_t ins = &vm->trace->entry[vm->pc];
            mr_object_t *regs    =  vm->regs->entry;
            if (ins->ins_code != INS_CODE_APPLY_PREPARE)
                break;
            mr_vm_uword_t argc = ins->apply_prepare.length;
#define ARGS(i) (regs[ins[i].apply_prepare.src])
            mr_object_t f = regs[ins->apply_prepare.src];
            if (f == lt)
            {
                mr_object_t a1 = ARGS(1);
                mr_object_t a2 = ARGS(2);
                if (MR_IS_INT(a1) && MR_IS_INT(a2))
                {
                    r = MR_INT_UNBOX(a1) <
                        MR_INT_UNBOX(a2) ?
                        MR_OBJECT_TRUE : MR_OBJECT_FALSE;
                }
                else r = MR_OBJECT_NULL;
            }
            else if (f == plus)
            {
                unsigned int result = 0;
                mr_vm_uword_t i;
                for (i = 1; i < argc; ++ i)
                {
                    mr_object_t a = ARGS(i);
                    if (MR_IS_INT(a))
                        result += MR_INT_UNBOX(a);
                }
                r = MR_INT_BOX(result);
            }
            else if (f == print)
            {
                // fprintf(stderr, "print\n");
                r = MR_OBJECT_NULL;
            }
            else r = MR_OBJECT_NULL;
            vm->pc += argc;
#undef ARGS
        }
        else
        {
            break;
        }
    }
    return 0;
}
