#ifndef __MR_IO_CHAR_STREAM_H__
#define __MR_IO_CHAR_STREAM_H__

#define MR_IO_CS_IN_PEEK    0
#define MR_IO_CS_IN_ADVANCE 1
typedef int(*mr_io_char_stream_in_f) (void*,int);
typedef int(*mr_io_char_stream_out_f)(void*,int);

#define MR_IO_CS_READ_U(in, data, var) (mr_io_cs_read_u(in, data, &(var), sizeof(var)))

int mr_io_cs_read_u(mr_io_char_stream_in_f in, void *data, void *result, unsigned int size);
int mr_io_cs_read_buf(mr_io_char_stream_in_f in, void *data, void *result, unsigned int size);
int mr_io_cs_read_u8(mr_io_char_stream_in_f in, void *data, uint8_t *result);
int mr_io_cs_read_u16(mr_io_char_stream_in_f in, void *data, uint16_t *result);
int mr_io_cs_read_u32(mr_io_char_stream_in_f in, void *data, uint32_t *result);

#endif
