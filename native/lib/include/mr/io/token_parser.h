#ifndef __MR_IO_TOKEN_PARSER_H__
#define __MR_IO_TOKEN_PARSER_H__

#include "char_stream.h"

#define MR_TOKEN_EOF    0
#define MR_TOKEN_LC     1
#define MR_TOKEN_RC     2
#define MR_TOKEN_SYMBOL 3
#define MR_TOKEN_STRING 4

#define MR_CHAR_IS_SEPARATOR(c) ((c) == '\n' || (c) == '\r' || (c) == ' ' || (c) == '\t')
#define MR_CHAR_IS_NUMERIC(c)   ((c) <= '9' && (c) >= '0')
#define MR_CHAR_IS_NEWLINE(c)   ((c) == '\n' || (c) == '\r')

#define MR_TOKEN_CHAR_NUMERIC_CONSTANT '#'
#define MR_TOKEN_CHAR_MINUS   '-'
#define MR_TOKEN_CHAR_DOT     '.'
#define MR_TOKEN_CHAR_LC      '('
#define MR_TOKEN_CHAR_RC      ')'
#define MR_TOKEN_CHAR_QUOTE   '"'
#define MR_TOKEN_CHAR_ESCAPE  '\\'
#define MR_TOKEN_CHAR_COMMENT ';'

/* Parse a token from the input stream */
/* return the type of token, -1 mean some error */
/* Only symbol and stream has a result buffer */
int mr_io_cs_parse_token(mr_io_char_stream_in_f in, void *stream, char **result, unsigned int *size);

#endif
