#ifndef BUFFER_H_INCLUDED
#define BUFFER_H_INCLUDED

#define BUFFER_DEFAULT_BUFFER_SIZE 4096
#define BUFFER_MAX_DUP_SIZE        65536

struct Buffer
{
    size_t used,
           allocated;

    char   *data;
};

typedef struct Buffer Buffer;

Buffer *buffer_init();
void    buffer_append(Buffer *b, const char *data, size_t size);
void    buffer_zero_terminate(Buffer *b);

#define buffer_get_data(B) ((B)->data)
#define buffer_get_size(B) ((B)->used)
#define buffer_clear(B)    do { (B)->used = 0; } while(0)
#define buffer_append_buffer(A,B) buffer_append(A,(B)->data,(B)->used)
#endif // BUFFER_H_INCLUDED
