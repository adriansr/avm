#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"


Buffer *buffer_init()
{
    Buffer *b = malloc(sizeof(Buffer));

    if (b)
    {
        b->data = malloc(BUFFER_DEFAULT_BUFFER_SIZE);

        if (b->data)
        {
            b->used      = 0;
            b->allocated = BUFFER_DEFAULT_BUFFER_SIZE;

            return b;
        }
        else
        {
            free(b);
            b = NULL;
        }
    }
    
    fprintf(stderr,"Not enough memory for buffer_init()!\n");
    exit(7);
}

void buffer_append(Buffer *b, const char *data, size_t size)
{
    size_t want = b->used + size;

    if (want > b->allocated)
    {
        while (want > b->allocated)
        {
            if (b->allocated < BUFFER_MAX_DUP_SIZE)
                b->allocated *= 2;
            else
                b->allocated += BUFFER_MAX_DUP_SIZE;
        }

        b->data = realloc(b->data, b->allocated);

        if (!b->data)
        {
            fprintf(stderr,"Realloc of %lu bytes failed!\n",
                    b->allocated);
            exit(7);
            return;
        }
    }

    memcpy( &b->data[b->used], data, size);
    b->used += size;
}

