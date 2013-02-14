#include "../internal.h"

int
mr_io_cs_parse_token(mr_io_char_stream_in_f in, void *stream, char **buf, unsigned int *size)
{
    int          type;
    char        *token_buf;
    unsigned int token_len;
    unsigned int token_buf_alloc = 0;

    type = -1;
    token_buf = NULL;
    token_len = 0;

#define EXPAND_TOKEN                                                    \
    while (token_buf_alloc < token_len)                                 \
    {                                                                   \
        token_buf_alloc = (token_buf_alloc << 1) + 1;                   \
        if (token_buf == NULL)                                          \
            token_buf = (char *)MR_MALLOC(sizeof(char) * token_buf_alloc); \
        else token_buf = (char *)MR_REALLOC(token_buf, sizeof(char) * token_buf_alloc); \
        if (token_buf == NULL) return -1;                               \
    }
    
    while (1)
    {
        int now = in(stream, MR_IO_CS_IN_PEEK);
        if (now < 0)
        {
            if (token_len > 0)
                type = MR_TOKEN_SYMBOL;
            else type = MR_TOKEN_EOF;
            break;
        }

        if (MR_CHAR_IS_SEPARATOR(now))
        {
            while (1)
            {
                in(stream, MR_IO_CS_IN_ADVANCE);
                now = in(stream, MR_IO_CS_IN_PEEK);
                if (MR_CHAR_IS_SEPARATOR(now)) continue;
                break;
            }
            
            if (token_len > 0)
            {
                type = MR_TOKEN_SYMBOL;
                break;
            }
            else if (now < 0) break;
        }

        if (now == MR_TOKEN_CHAR_LC || now == MR_TOKEN_CHAR_RC)
        {
            if (token_len > 0)
            {
                type = MR_TOKEN_SYMBOL;
                break;
            }
            else
            {
                in(stream, MR_IO_CS_IN_ADVANCE);
                
                if (now == MR_TOKEN_CHAR_LC)
                    type = MR_TOKEN_LC;
                else type = MR_TOKEN_RC;
                
                break;
            }
        }

        if (now == MR_TOKEN_CHAR_COMMENT)
        {
            in(stream, MR_IO_CS_IN_ADVANCE);
            while (1)
            {
                now = in(stream, MR_IO_CS_IN_PEEK);
                if (now < 0 || MR_CHAR_IS_NEWLINE(now))
                {
                    while (1)
                    {
                        now = in(stream, MR_IO_CS_IN_PEEK);
                        if (now < 0 || !MR_CHAR_IS_NEWLINE(now))
                            break;
                        else in(stream, MR_IO_CS_IN_ADVANCE);
                    }
                    break;
                }
                else in(stream, MR_IO_CS_IN_ADVANCE);
            }
            continue;
        }

        if (now == MR_TOKEN_CHAR_QUOTE)
        {
            if (token_len > 0)
            {
                type = MR_TOKEN_SYMBOL;
                break;
            }
            else
            {
                in(stream, MR_IO_CS_IN_ADVANCE);
                type = MR_TOKEN_STRING;
                
                while (1)
                {
                    now = in(stream, MR_IO_CS_IN_ADVANCE);
                    if (now < 0 || now == MR_TOKEN_CHAR_QUOTE)
                    {
                        break;
                    }
                    else if (now == MR_TOKEN_CHAR_ESCAPE)
                    {
                        now = in(stream, MR_IO_CS_IN_ADVANCE);
                        if (now < 0)
                        {
                            break;
                        }
                        
                        ++ token_len;
                        EXPAND_TOKEN;

                        switch (now)
                        {
                        case 'n':
                            token_buf[token_len - 1] = '\n';
                            break;

                        case 'r':
                            token_buf[token_len - 1] = '\r';
                            break;

                        default:
                            token_buf[token_len - 1] = now;
                            break;
                        }
                    }
                    else
                    {
                        ++ token_len;
                        EXPAND_TOKEN;
                        token_buf[token_len - 1] = now;
                    }
                }

                break;
            }
        }

        ++ token_len;
        EXPAND_TOKEN;
        token_buf[token_len - 1] = now;

        in(stream, MR_IO_CS_IN_ADVANCE);
    }
#undef EXPAND_TOKEN

    if ((*size = token_len) > 0)
    {
        *buf = (char *)MR_REALLOC(token_buf, token_len);
    }
    else if (token_buf != NULL)
        MR_FREE(token_buf);
    return type;
}
