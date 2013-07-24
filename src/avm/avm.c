#include "avm/internals.h"

#include <stdlib.h>
#include <string.h>

AVM avm_init()
{
    AVM vm = ALLOC_OPAQUE_STRUCT(AVM);

    if (vm != NULL)
    {
        memset(vm,0,sizeof(*vm));
        
        vm->version    = AVM_VERSION;
        vm->hash_fn    = _avm_default_hash;
        vm->hash_seed  = AVM_DEFAULT_HASH_SEED;
        vm->error_code = AVM_NO_ERROR;
        vm->error_pos  = (size_t)-1;
    }

    return vm;
}

void avm_free(AVM vm)
{
    if (vm != NULL)
    {
        if (vm->runtime.acc)
        {
            avm_object_free(vm,vm->runtime.acc);
            vm->runtime.acc = NULL;
        }
        
        if (vm->runtime.vars)
        {
            avm_dict_free(vm->runtime.vars);
            vm->runtime.vars = NULL;
        }
        
        if (vm->integer_pool)
        {
            avm_pool_free(vm->integer_pool);
        }

        free(vm);
        vm = NULL;
    }
}

void avm_set_hash_fn(AVM       vm,
                     AVMHashFn h)
{
    if (h != NULL)
    {
        vm->hash_fn = h;
    }
    else
    {
        vm->hash_fn = _avm_default_hash;
    }
}

uint16_t avm_version(AVM vm)
{
    return vm->version;
}

uint16_t avm_error(AVM vm)
{
    return vm->error_code;
}

size_t  avm_error_position(AVM vm)
{
    return vm->error_pos;
}

void avm_set_hash_seed(AVM     vm,
                       AVMHash seed)
{
    vm->hash_seed = seed;
}

AVMHash avm_hash(AVM vm, const char *data, size_t len)
{
    return (* vm->hash_fn)(data,len,vm->hash_seed);
}

void   _avm_set_error(AVM vm, uint16_t code, size_t pos)
{
    vm->error_code = code;
    vm->error_pos  = pos;
}

uint32_t avm_stats_icount(AVM vm)
{
    return vm->icount;
}

AVMError avm_tune(AVM vm, uint32_t integer_pool_size)
{
    if (vm->integer_pool)
    {
        avm_pool_free(vm->integer_pool);
    }
    
    if (integer_pool_size)
    {
        vm->integer_pool = avm_pool_init(integer_pool_size);
        if (!vm->integer_pool)
            return AVM_ERROR_NO_MEM;
    }
    else
    {
        vm->integer_pool = 0;
    }

    return AVM_NO_ERROR;
}

/*AVMInteger _avm_get_integer(AVM vm, uint32_t value)
{
    
    return (vm->integer_pool)? avm_pool_get_integer(vm->integer_pool, value)
                             : avm_create_integer(value);
}

void _avm_release_integer(AVM vm,AVMInteger ii)
{
    if (vm->integer_pool)
        avm_pool_release(vm->integer_pool,(AVMObject)ii);
    else
        avm_object_free((AVMObject)ii);
}

void _avm_release_object(AVM vm, AVMObject o)
{
    switch (o->type)
    {
        case AVMTypeInteger:
            _avm_release_integer(vm, (AVMInteger)o);
            break;

        default:
            avm_object_free(o);
    }
}*/

