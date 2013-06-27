#include "avm/internals.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


AVMStack avm_stack_init()
{
    return avm_stack_init_with_reserve(0);
}

AVMStack avm_stack_init_with_reserve(uint32_t entries)
{
    AVMStack s = ALLOC_OPAQUE_STRUCT(AVMStack);

    if (s!=NULL)
    {
        s->used     = 0;
        s->reserved = entries;
        if (entries)
        {
            s->ptr = malloc( entries * sizeof(AVMObject) );

            if (s->ptr == NULL)
            {
                free(s);
                return NULL;
            }
        }
        else
        {
            s->ptr = NULL;
        }
    }

    return s;
}

void avm_stack_free(AVMStack s)
{
    if (s!=NULL)
    {
        if (s->ptr != NULL)
        {
            size_t i;
            for (i=0;i<s->used;++i)
            {
                if (s->ptr[i]) 
                    avm_object_free(s->ptr[i]);
            }

            free(s->ptr);
        }

        free(s);
    }
}

static char _avm_stack_grow(AVMStack s)
{
    if (s->reserved)
    {
        if (s->reserved < AVM_STACK_DUP_LIMIT)
            s->reserved *= 2;
        else
            s->reserved += AVM_STACK_DUP_LIMIT;
    }
    else
        s->reserved = AVM_STACK_INITIAL_RESERVE;

    s->ptr = realloc(s->ptr, s->reserved * sizeof(AVMObject));
    if (!s->ptr)
    {
        s->used = s->reserved = 0;
        return 0;
    }

    return 1;
}

AVMError avm_stack_push(AVMStack s, AVMObject obj)
{
    if (obj != NULL)
    {
        if (s->used == s->reserved)
        {
            if (!_avm_stack_grow(s))
                return AVM_ERROR_NO_MEM;
        }

        s->ptr[s->used++] = obj; 
    }

    return AVM_NO_ERROR;
}

AVMObject avm_stack_at(AVMStack s, uint32_t pos)
{
    return (pos < s->used)? s->ptr[s->used - 1 - pos] : NULL;
}

AVMObject avm_stack_pop(AVMStack s)
{
    return s->used? s->ptr[ -- s->used] : NULL;
}

AVMError avm_stack_discard(AVMStack s, uint32_t n)
{
    if ( n <= s->used )
    {
        s->used -= n;
        return AVM_NO_ERROR;
    }

    s->used = 0;
    return AVM_ERROR_INVALID_DISCARD;
}

uint32_t avm_stack_size(AVMStack s)
{
    return s->used;
}

void _avm_stack_set(AVMStack s, uint32_t n, AVMObject o)
{
    if (n < s->used)
    {
        s->ptr[s->used - 1 - n] = o;
    }
}

void avm_stack_print(AVMStack s)
{
    uint32_t i,
             tmp,
             siz = avm_stack_size(s);

    printf("Stack [%p] #elems %u\n", s, siz);
    
    for (i=0;i<siz;++i)
    {
        AVMObject o = avm_stack_at(s,i);

        printf(" at %u [%p] ", i, o);

        switch( (AVMType)o->type )
        {
            case AVMTypeObject:
                printf("*** object ***");
                break;

            case AVMTypeInteger:
                tmp = avm_integer_get((AVMInteger)o);
                printf("int  %d [0x%x]", (int)tmp, tmp);
                break;

            case AVMTypeString:
                tmp = avm_string_length((AVMString)o);
                printf("str  (%u) \"", tmp);
                fwrite(avm_string_data((AVMString)o), tmp, 1, stdout);
                printf("\"");
                break;

            case AVMTypeCode:
                tmp = avm_string_length((AVMString)o);
                printf("code {%u bytes}", tmp);
                break;

            case AVMTypeRef:
                printf("ref  @<%08x>",avm_ref_get((AVMRef)o));
                break;

            default:
                printf("*** unknown %u ***", o->type);
        }

        printf("\n");
    }
}

