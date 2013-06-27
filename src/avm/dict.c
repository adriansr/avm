#include "avm/internals.h"

#include <stdlib.h>
#include <string.h>


AVMDict avm_dict_init(uint8_t size_exp)
{
    uint32_t size = size_exp;

    if      (size < AVM_DICT_MIN_SIZE_EXP)
    {
        size = size==0? AVM_DICT_DEFAULT_SIZE_EXP : AVM_DICT_MIN_SIZE_EXP;
    }
    else if (size > AVM_DICT_MAX_SIZE_EXP) size = AVM_DICT_MAX_SIZE_EXP;
    
    size = 1 << size;

    AVMDict o = ALLOC_OPAQUE_STRUCT_WITH_EXTRA(AVMDict,
                                               sizeof(struct _AVMDictEntry*)*size);

    if (o != NULL)
    {
        o->size  = size;
        o->mask  = size - 1; 
        o->count = 0;

        memset(&o->dict, 0, size);
    }

    return o;
}

static AVMError _store_key(AVMHash key, AVMObject value,
                           struct _AVMDictEntry **pPrev,
                           struct _AVMDictEntry *next)
{
    struct _AVMDictEntry *entry = malloc( sizeof(struct _AVMDictEntry) );

    if (!entry)
    {
        return AVM_ERROR_NO_MEM;
    }

    entry->key   = key;
    entry->value = value;
    entry->next  = next;
    *pPrev       = entry;

    return AVM_NO_ERROR;
}

AVMError avm_dict_set (AVMDict dict, AVMHash key, AVMObject value)
{
    uint32_t pos = key & dict->mask;
    struct _AVMDictEntry **ptr = & dict->dict[pos];

    if (!*ptr)
    {
        return _store_key(key, value, ptr, NULL);
    }
    else
    {
        struct _AVMDictEntry **prev = ptr;
        struct _AVMDictEntry  *cur  = *ptr;

        while (cur != NULL && cur->key < key)
        {
            prev = &cur->next;
            cur  =  cur->next;
        }

        if (cur != NULL)
        {
            if (cur->key == key)
            {
                if (cur->value)
                    avm_object_free(cur->value);
                
                cur->value = value;

                return AVM_NO_ERROR;
            }

            return _store_key(key,value, prev, cur);
        }
        else
        {
            return _store_key(key, value, prev, NULL);
        }
    }

}
