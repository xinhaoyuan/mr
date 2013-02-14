#include "xstring.h"

xstring_t
xstring_from_cstr(const char *cstr, mr_uintptr_t len)
{
    xstring_t result = (xstring_t)MR_MALLOC(sizeof(struct xstring_s));
    if (result == NULL) return NULL;

    if (len == XSTRING_LEN_UNDEFINED) len = MR_STRLEN(cstr);
    result->cstr = (char *)MR_MALLOC(sizeof(char) * (len + 1));

    if (result->cstr == NULL)
    {
        MR_FREE(result);
        return NULL;
    }
    
    result->len = len;
    result->ref = 1;

    MR_MEMCPY(result->cstr, cstr, sizeof(char) * len);
    result->cstr[len] = 0;

    return result;
}

int
xstring_equal_cstr(xstring_t string, const char *cstr, mr_uintptr_t len)
{
    if (len == XSTRING_LEN_UNDEFINED) len = MR_STRLEN(cstr);

    if (string->len != len) return 0;
    else return MR_MEMCMP(string->cstr, cstr, len) == 0;
}

int
xstring_equal(xstring_t a, xstring_t b)
{
    if (a->len != b->len) return 0;
    else return MR_MEMCMP(a->cstr, b->cstr, a->len) == 0;
}

char *
xstring_cstr(xstring_t string)
{
    return string->cstr;
}

mr_uintptr_t
xstring_len(xstring_t string)
{
    return string->len;
}

void
xstring_get(xstring_t string)
{
    ++ string->ref;
}

void
xstring_put(xstring_t string)
{
    if (-- string->ref == 0)
    {
        MR_FREE(string->cstr);
        MR_FREE(string);
    }
}
