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
            AVMDict     vars;
        } runtime;

        /* stats */
        uint32_t icount; /* instruction count */
    };
    
#   define AVM_DEFAULT_HASH_SEED 0x873d1ae5

    AVMHash _avm_default_hash(const char *, size_t, AVMHash);
    AVMHash _avm_hash        (AVM, const char*, size_t);
    void    _avm_set_error   (AVM, uint16_t, size_t);

/*
 * OBJECTS
 */

    struct _AVMObject
    {
        uint8_t type;
    };

    struct _AVMInteger
    {
        uint8_t  type;
        int32_t  value;
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
    
    void _avm_stack_set(AVMStack s, uint32_t n, AVMObject o);

/*
 * DICT
 */
#define AVM_DICT_MIN_SIZE_EXP     4
#define AVM_DICT_DEFAULT_SIZE_EXP 10
#define AVM_DICT_MAX_SIZE_EXP     16
    
    struct _AVMDictEntry
    {
        AVMHash   key;
        AVMObject value;
        struct _AVMDictEntry *next;
    };

    struct _AVMDict
    {
        uint32_t     size,
                     mask,
                     count;
        struct _AVMDictEntry* dict[];
    };

#endif /* AVM_INTERNALS_INCLUDED */
