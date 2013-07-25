
#include "avm/internals.h"

#include <stdlib.h>
#include <string.h>


static AVMString _create_buffer_type(AVMType t, const char *data, uint32_t size)
{
    AVMString o = ALLOC_OPAQUE_STRUCT_WITH_EXTRA(AVMString,size);

    if (o)
    {
        o->type   = t;
        o->length = size;
        if (size)
        {
            if (data != NULL)
                memcpy(o->data, data, size);
            else
                memset(o->data, 0, size);
        }
    }

    return o;
}

AVMMark avm_create_mark()
{
    AVMMark o = ALLOC_OPAQUE_STRUCT(AVMMark);

    if (o != NULL)
    {
        o->type  = AVMTypeMark;
    }

    return o;
}


AVMInteger  _avm_create_integer(int32_t value)
{
    AVMInteger o = ALLOC_OPAQUE_STRUCT(AVMInteger);

    if (o != NULL)
    {
        o->type  = AVMTypeInteger;
        o->value = value;
    }

    return o;
}

AVMInteger avm_create_integer(AVM vm, int32_t value)
{
    return avm_pool_get_integer(vm->integer_pool, value);
}

AVMString avm_create_cstring(const char *s)
{
    return _create_buffer_type(AVMTypeString,s,s?strlen(s):0);
}

AVMString avm_create_string(const char *data, uint32_t size)
{
    return _create_buffer_type(AVMTypeString, data, size);
}

AVMString avm_create_string_empty(uint32_t size)
{
    return _create_buffer_type(AVMTypeString, NULL, size);
}

AVMCode avm_create_code(const char *ptr, uint32_t size)
{
    return (AVMCode) _create_buffer_type(AVMTypeCode,ptr,size);
}

AVMRef avm_create_ref(uint32_t hash)
{
    AVMRef o = ALLOC_OPAQUE_STRUCT(AVMRef);

    if (o != NULL)
    {
        o->type = AVMTypeRef;
        o->ref  = hash;
    }

    return o;
}

AVMExternal avm_create_external(AVMExternalType f)
{
    AVMExternal o = ALLOC_OPAQUE_STRUCT(AVMExternal);

    if (o != NULL)
    {
        o->type = AVMTypeExternal;
        o->ptr  = f;
    }

    return o;
}


void avm_object_free(AVM vm, AVMObject o)
{
    if (o)
    {
        AVMPool pool = NULL;

        if (vm)
        {
            switch(o->type)
            {
                case AVMTypeInteger:
                    pool = vm->integer_pool;
                    break;
            }
        }

        avm_pool_release(pool, o);
    }
}

AVMType avm_object_type(AVMObject o)
{
    return o->type;
}

int32_t avm_integer_get(AVMInteger o)
{
    return o->value;
}

uint32_t avm_ref_get(AVMRef o)
{
    return o->ref;
}

uint32_t avm_string_length(AVMString o)
{
    return o->length;
}

const char* avm_string_data(AVMString o)
{
    return o->length? o->data : NULL;
}

size_t _avm_object_raw_size(AVMObject o)
{
    switch((AVMType)o->type)
    {
        case AVMTypeMark:
            return sizeof(struct _AVMMark);

        case AVMTypeInteger:
            return sizeof(struct _AVMInteger);

        case AVMTypeString:
            return sizeof(struct _AVMString) + ((AVMString)o)->length;

        case AVMTypeCode:
            return sizeof(struct _AVMString) + ((AVMCode)o)->length;

        case AVMTypeRef:
            return sizeof(struct _AVMRef);
        
        case AVMTypeExternal:
            return sizeof(struct _AVMExternal);

        default:
            return 0;
    }
}

AVMObject avm_object_copy(AVMObject o)
{
    AVMObject copy = NULL;
    size_t s;
    
    if (o && (s=_avm_object_raw_size(o)) )
    {   
        copy = malloc(s);

        if (copy)
        {
            memcpy(copy,o,s);
        }
    }

    return copy;
}


