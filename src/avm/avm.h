#ifndef AVM_H_INCLUDED
#define AVM_H_INCLUDED

#define AVM_VERSION 0x0001

#include <stdint.h>
#include <string.h>
/* 
 * ERRORS
 */

#define AVM_NO_ERROR             0
#define AVM_NO_ERROR_EXIT        1

/* system errors 0x00xx */
#define AVM_ERROR_INVALID_ARG    0x0001
#define AVM_ERROR_NO_MEM         0x0002

/* execution errors 0x01xx */
#define AVM_ERROR_NULL_OPCODE    0x0100
#define AVM_ERROR_INVALID_OPCODE 0x0101
#define AVM_ERROR_NO_CODE        0x0102
#define AVM_ERROR_BAD_VLINT      0x0103
#define AVM_ERROR_STR_TRUNCATED  0x0104
#define AVM_ERROR_CODE_TRUNCATED 0x0105
#define AVM_ERROR_REF_TRUNCATED  0x0106
#define AVM_ERROR_REF_NOT_BIND   0x0107
#define AVM_ERROR_RANGE_CHECK    0x0108
#define AVM_ERROR_UNIMPLEMENTED  0x0109
#define AVM_ERROR_BAD_INCREMENT  0x010a
#define AVM_ERROR_STACK_RANGE    0x010b
#define AVM_ERROR_DELTA_RANGE    0x010c
#define AVM_ERROR_CHAR_VALUE     0x010d
#define AVM_ERROR_MARK_NOT_FOUND 0x010e
#define AVM_ERROR_STRING_RANGE   0x010f
#define AVM_ERROR_ACC_NOT_SET    0x0110

/* inconsistency errors 0x02xx */
#define AVM_ERROR_INVALID_DISCARD 0x0200

/* stack argument errors 03xx */
#define AVM_ERROR_NOT_ENOUGH_ARGS 0x0300
#define AVM_ERROR_WRONG_TYPE      0x0301
#define AVM_ERROR_REF_EXPECTED    0x0302
#define AVM_ERROR_DIFFERENT_TYPES 0x0303 // comparing different types
#define AVM_ERROR_TYPE_NOT_EXEC   0x0304
#define AVM_ERROR_NEGATIVE_TIMES  0x0305

typedef struct _AVM*      AVM;
typedef struct _AVMStack* AVMStack;
typedef struct _AVMDict*  AVMDict;

typedef uint32_t AVMHash;
typedef int      AVMError;

typedef enum {
    AVMTypeObject = 0, /* invalid */
    AVMTypeInteger,
    AVMTypeString,
    AVMTypeCode,
    AVMTypeRef,
    AVMTypeMark,
    AVMTypeExternal,
} AVMType;

typedef struct _AVMObject*  AVMObject;
typedef struct _AVMString*  AVMString;
typedef struct _AVMInteger* AVMInteger;
typedef struct _AVMString*  AVMCode;
typedef struct _AVMRef*     AVMRef;
typedef struct _AVMMark*    AVMMark;
typedef struct _AVMExternal* AVMExternal;

typedef AVMHash (* AVMHashFn) (const char *, size_t, AVMHash);

typedef AVMError (* AVMExternalType) (AVM, AVMStack);

/*
 * VM
 */
    
    /* (de)init */
    AVM  avm_init();
    void avm_free(AVM vm);
    
    /* running */
    AVMError avm_run(AVM vm, 
                     const char *code, size_t size,
                     AVMStack s);

    /* tunning */
    void avm_set_hash_fn  (AVM vm, AVMHashFn h);
    void avm_set_hash_seed(AVM vm, AVMHash   seed);
    AVMError avm_tune(AVM vm, uint32_t integer_pool_size);

    /* misc */
    uint16_t avm_version(AVM vm);
    uint32_t avm_stats_icount(AVM vm);
    AVMHash avm_hash(AVM, const char*, size_t);
    
    /* vars */
    AVMError avm_set_var(AVM,AVMHash,AVMObject);
    AVMError avm_set_var_by_name(AVM,const char*, AVMObject);

    /* errors */
    uint16_t avm_error(AVM vm);
    size_t   avm_error_position(AVM vm);

    /*
     * OBJECTS
     */

    AVMInteger  avm_create_integer(AVM vm, int32_t value);
    AVMString   avm_create_cstring(const char *s);
    AVMString   avm_create_string (const char *data, uint32_t size);
    AVMString   avm_create_string_empty(uint32_t size);
    AVMCode     avm_create_code   (const char *ptr,  uint32_t size);
    AVMRef      avm_create_ref    (uint32_t hash);
    AVMMark     avm_create_mark   ();
    AVMExternal avm_create_external(AVMExternalType f);

    void        avm_object_free   (AVM vm, AVMObject o);
    AVMObject   avm_object_copy   (AVMObject o);
    AVMType     avm_object_type   (AVMObject o);
    int32_t     avm_integer_get   (AVMInteger o);
    uint32_t    avm_ref_get       (AVMRef o);
    uint32_t    avm_string_length (AVMString o);
    const char* avm_string_data   (AVMString o);
    /*
     * STACK
     */

    AVMStack avm_stack_init();
    AVMStack avm_stack_init_with_reserve(uint32_t entries);
    void     avm_stack_free();

    AVMError  avm_stack_push(AVMStack s, AVMObject obj);
    AVMObject avm_stack_at(AVMStack s, uint32_t pos);
    AVMObject avm_stack_pop(AVMStack s);
    AVMError  avm_stack_discard(AVMStack s, uint32_t n);
    uint32_t  avm_stack_size(AVMStack s);
    void      avm_stack_clear(AVMStack s);

    void avm_stack_print(AVMStack s);

    /*
     * DICT
     */
    AVMDict   avm_dict_init(uint8_t size_exp);
    AVMError  avm_dict_set (AVMDict dict, AVMHash key, AVMObject value);
    AVMObject avm_dict_get (AVMDict dict, AVMHash key);
    void      avm_dict_free(AVMDict dict);
    AVMError  avm_dict_remove(AVMDict dict, AVMHash key);
    /*
     * EXECUTION
     */

#endif /* AVM_H_INCLUDED */
