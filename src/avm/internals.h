#ifndef AVM_INTERNALS_INCLUDED
#define AVM_INTERNALS_INCLUDED

#include "avm/avm.h"

#define ALLOC_OPAQUE_STRUCT(TYPE) ALLOC_OPAQUE_STRUCT_WITH_EXTRA(TYPE,0)
#define ALLOC_OPAQUE_STRUCT_WITH_EXTRA(TYPE,EXTRA) ((TYPE)malloc((EXTRA)+sizeof(struct _##TYPE)))
/*
 * VM
 */

    struct _AVM
    {
        uint16_t version;

        /* hash settings */
        AVMHashFn hash_fn;
        AVMHash   hash_seed;
        
        /* error settings */
        AVMError  error_code;
        size_t    error_pos;

        /* runtime state */
        struct
        {
            const char *code;
            size_t      pos,
                        size;
            AVMStack    stack;
        } runtime;
    };
    
#   define AVM_DEFAULT_HASH_SEED 0x873d1ae5

    AVMHash _avm_default_hash(const char *, size_t, AVMHash);
    AVMHash _avm_hash        (AVM, const char*, size_t);
    void    _avm_set_error   (AVM, uint16_t, size_t);

/*
 * OBJECTS
 */
#   define AVM_TYPE_INTEGER 0x01
#   define AVM_TYPE_STRING  0x02

    struct _AVMObject
    {
        uint8_t type;
    };

    struct _AVMInteger
    {
        uint8_t  type;
        uint32_t value;
    };

    struct _AVMString
    {
        uint8_t  type;
        uint32_t length;
        char     data[];
    };
    
    struct _AVMRef
    {
        uint8_t  type;
        uint32_t ref;
    };
    
    size_t _avm_object_raw_size(AVMObject);

/*
 * STACK
 */
#define AVM_STACK_INITIAL_RESERVE 16
#define AVM_STACK_DUP_LIMIT       65536


    struct _AVMStack
    {
        uint32_t  used,
                  reserved;
        AVMObject *ptr;
    };

#endif /* AVM_INTERNALS_INCLUDED */
