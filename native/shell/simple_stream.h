#ifndef __MR_SHELL_FSTREAM_H__
#define __MR_SHELL_FSTREAM_H__

#include <stdio.h>

struct fstream_s
{
    FILE *file;
    int buf;
};

typedef struct fstream_s fstream_s;
typedef struct fstream_s *fstream_t;

void fstream_open(fstream_t stream, FILE *file);
void fstream_close(fstream_t stream);
int  fstream_in(fstream_t stream, int advance);

typedef struct memstream_s *memstream_t;
typedef struct memstream_s
{
    unsigned char *buf;
    unsigned int   size;
    unsigned int   pos;
} memstream_s;

void memstream_open(memstream_t stream, void *buf, unsigned int size);
int  memstream_in(memstream_t stream, int advance);

typedef struct neststream_s *neststream_t;
typedef struct neststream_s
{
    int (*in)(void*,int);
    void *stream;
    unsigned int limit;
    unsigned int pos;
} neststream_s;

void neststream_open(neststream_t stream, int (*__in)(void*,int), void *__stream, unsigned int size);
int  neststream_in(neststream_t stream, int advance);

#endif
