#include "simple_stream.h"

#define BUF_END (-1)
#define BUF_ERROR (-2)
#define BUF_EMPTY (-3)

void
fstream_open(fstream_t stream, FILE *file)
{
    stream->file = file;
    stream->buf = file == NULL ? BUF_ERROR : BUF_EMPTY;
}

void
fstream_close(fstream_t stream)
{
    if (stream->file != NULL) fclose(stream->file);
    stream->file = NULL;
    stream->buf = BUF_ERROR;
}

int
fstream_in(fstream_t stream, int advance)
{
    int r;
    if (advance)
    {
        if (stream->buf == BUF_EMPTY)
            r = fgetc(stream->file);
        else
        {
            r = stream->buf;
            if (stream->buf != BUF_ERROR)
                stream->buf = BUF_EMPTY;
        }
    }
    else
    {
        if (stream->buf == BUF_EMPTY)
        {
            stream->buf = fgetc(stream->file);
            if (stream->buf < 0) stream->buf = BUF_END;
        }
        
        r = stream->buf;
    }
    return r;
}

void
memstream_open(memstream_t stream, void *buf, unsigned int size)
{
    stream->buf = buf;
    stream->size = size;
    stream->pos = 0;
}

int
memstream_in(memstream_t stream, int advance)
{
    int r = stream->pos < stream->size ?
        stream->buf[stream->pos] :
        BUF_EMPTY;
    
    if (advance && r >= 0)
        ++ stream->pos;

    return r;
}


void
neststream_open(neststream_t stream, int (*__in)(void*,int), void *__stream, unsigned int size)
{
    stream->in     = __in;
    stream->stream = __stream;
    stream->limit  = size;
    stream->pos    = 0;
}

int
neststream_in(neststream_t stream, int advance)
{
    int r = stream->pos < stream->limit ?
        stream->in(stream->stream, advance)
        : BUF_EMPTY;
    
    if (advance && r >= 0)
        ++ stream->pos;

    return r;
}
