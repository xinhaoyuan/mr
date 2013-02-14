#include "../internal.h"

int
mr_io_cs_read_u(mr_io_char_stream_in_f in, void *data, void *result, unsigned int size)
{
    int c;
    unsigned int i;
    for (i = 0; i < size; ++ i)
    {
        c = in(data, MR_IO_CS_IN_ADVANCE);
        if (c < 0)
            return -1;
        ((uint8_t *)result)[i] = c;
    }
    return 0;
}


int
mr_io_cs_read_buf(mr_io_char_stream_in_f in, void *data, void *result, unsigned int size)
{
    int c;
    unsigned int i;
    for (i = 0; i < size; ++ i)
    {
        c = in(data, MR_IO_CS_IN_ADVANCE);
        if (c < 0)
            return -1;
        ((uint8_t *)result)[i] = c;
    }
    return 0;
}

int
mr_io_cs_read_u8(mr_io_char_stream_in_f in, void *data, uint8_t *result)
{
    int c = in(data, MR_IO_CS_IN_ADVANCE);
    if (c < 0)
        return -1;
    result[0] = c;
    
    return 0;
}

int
mr_io_cs_read_u16(mr_io_char_stream_in_f in, void *data, uint16_t *result)
{
    int c = in(data, MR_IO_CS_IN_ADVANCE);
    if (c < 0)
        return -1;
    ((uint8_t *)result)[0] = c;

    c = in(data, MR_IO_CS_IN_ADVANCE);
    if (c < 0)
        return -1;
    ((uint8_t *)result)[1] = c;

    return 0;
}

int
mr_io_cs_read_u32(mr_io_char_stream_in_f in, void *data, uint32_t *result)
{
    int c = in(data, MR_IO_CS_IN_ADVANCE);
    if (c < 0)
        return -1;
    ((uint8_t *)result)[0] = c;

    c = in(data, MR_IO_CS_IN_ADVANCE);
    if (c < 0)
        return -1;
    ((uint8_t *)result)[1] = c;

    c = in(data, MR_IO_CS_IN_ADVANCE);
    if (c < 0)
        return -1;
    ((uint8_t *)result)[2] = c;

    c = in(data, MR_IO_CS_IN_ADVANCE);
    if (c < 0)
        return -1;
    ((uint8_t *)result)[3] = c;

    return 0;
}
