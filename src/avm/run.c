#include "avm/internals.h"
#include "avm/opcodes.h"

#include <stdlib.h>
#include <string.h>

static AVMError _avm_read_vlint(AVM vm, uint32_t *pVal)
{
    uint32_t val = 0;
    const char *code = vm->runtime.code;

    size_t i     = 0,
           limit = vm->runtime.size - vm->runtime.pos; 

    code += vm->runtime.pos;

    if (limit>5) limit = 5;

    while (i<limit)
    {
        uint8_t v = (uint8_t)code[i++];

        val = (val<<7) | (v&0x7f);

        if (v & 0x80)
        {
            *pVal            = val;
            vm->runtime.pos += i;
            return AVM_NO_ERROR;
        }
    }

    *pVal  = 0;
    /* *pPos += i; */

    return AVM_ERROR_BAD_VLINT;
}

static AVMError _parse_PushInt(AVM vm)
{
    uint32_t value;
    AVMError err = _avm_read_vlint(vm,&value);

    if (err != AVM_NO_ERROR)
    {
        return err;
    }

    AVMInteger o = avm_create_integer((int32_t)value);

    if (o == NULL)
    {
        return AVM_ERROR_NO_MEM;
    }
    
    return avm_stack_push(vm->runtime.stack, (AVMObject)o);
}

static AVMError _parse_PushNegInt(AVM vm)
{
    uint32_t value;
    AVMError err = _avm_read_vlint(vm,&value);

    if (err != AVM_NO_ERROR)
    {
        return err;
    }

    AVMInteger o = avm_create_integer(-(int32_t)value);

    if (o == NULL)
    {
        return AVM_ERROR_NO_MEM;
    }
    
    return avm_stack_push(vm->runtime.stack, (AVMObject)o);
}

static AVMError _parse_PushRef(AVM vm)
{
    uint32_t value;
    
    if (vm->runtime.pos + 4 > vm->runtime.size)
    {
        return AVM_ERROR_REF_TRUNCATED;
    }
    
    uint8_t *p = (uint8_t*)&vm->runtime.code[vm->runtime.pos];

    value = (p[0]<<24)
          | (p[1]<<16)
          | (p[2]<<8)
          | (p[3]);
    
    vm->runtime.pos += 4;
    
    AVMRef o = avm_create_ref(value);

    if (o == NULL)
    {
        return AVM_ERROR_NO_MEM;
    }
    
    return avm_stack_push(vm->runtime.stack, (AVMObject)o);
}

static AVMError _run_subroutine(AVM vm, AVMCode code)
{
    const char *saved_code = vm->runtime.code;
    size_t      saved_pos  = vm->runtime.pos,
                saved_size = vm->runtime.size;

    AVMError err = avm_run(vm, code->data, code->length, vm->runtime.stack);

    vm->runtime.code = saved_code;
    vm->runtime.pos  = saved_pos;
    vm->runtime.size = saved_size;

    return err;
}

static AVMError _parse_PushRefVal(AVM vm)
{
    AVMHash hash;
    
    if (vm->runtime.pos + 4 > vm->runtime.size)
    {
        return AVM_ERROR_REF_TRUNCATED;
    }
    
    uint8_t *p = (uint8_t*)&vm->runtime.code[vm->runtime.pos];

    hash = (p[0]<<24)
         | (p[1]<<16)
         | (p[2]<<8)
         | (p[3]);
    
    vm->runtime.pos += 4;
    
    AVMDict dict = vm->runtime.vars;
    AVMObject o;

    if (!dict || !(o=avm_dict_get(dict, hash)) )
    {
        return AVM_ERROR_REF_NOT_BIND;
    }
    
    if (o->type != AVMTypeCode)
    {
        return avm_stack_push(vm->runtime.stack, o);
    }
    else
    {
        return _run_subroutine(vm, (AVMCode)o);
    }
}

static AVMError _parse_PushString(AVM vm)
{
    uint32_t length;
    AVMError err = _avm_read_vlint(vm,&length);

    if (err != AVM_NO_ERROR)
    {
        return err;
    }
    
    if (vm->runtime.pos + length > vm->runtime.size)
    {
        return AVM_ERROR_STR_TRUNCATED;
    }

    AVMString o = avm_create_string(&vm->runtime.code[vm->runtime.pos],
                                    length);

    if (o == NULL)
    {
        return AVM_ERROR_NO_MEM;
    }

    vm->runtime.pos += length;

    return avm_stack_push(vm->runtime.stack, (AVMObject)o);
}

static AVMError _parse_PushCode(AVM vm)
{
    uint32_t length;
    AVMError err = _avm_read_vlint(vm,&length);

    if (err != AVM_NO_ERROR)
    {
        return err;
    }
    
    if (vm->runtime.pos + length > vm->runtime.size)
    {
        return AVM_ERROR_CODE_TRUNCATED;
    }

    AVMCode o = avm_create_code(&vm->runtime.code[vm->runtime.pos],
                                length);

    if (o == NULL)
    {
        return AVM_ERROR_NO_MEM;
    }

    vm->runtime.pos += length;

    return avm_stack_push(vm->runtime.stack, (AVMObject)o);
}

static AVMError _parse_Add(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 2)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject b = avm_stack_at(s,0),
              a = avm_stack_at(s,1);

    if (a->type != AVMTypeInteger || a->type != b->type)
        return AVM_ERROR_WRONG_TYPE;

    ((AVMInteger)a)->value += ((AVMInteger)b)->value;
    
    avm_object_free(b);
    _avm_stack_set(s,1,a);

    return avm_stack_discard(s,1);
}

static AVMError _parse_Sub(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 2)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject b = avm_stack_at(s,0),
              a = avm_stack_at(s,1);

    if (a->type != AVMTypeInteger || a->type != b->type)
        return AVM_ERROR_WRONG_TYPE;

    ((AVMInteger)a)->value -= ((AVMInteger)b)->value;

    avm_object_free(b);
    _avm_stack_set(s,1,a);

    return avm_stack_discard(s,1);
}

static AVMError _parse_Div(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 2)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject b = avm_stack_at(s,0),
              a = avm_stack_at(s,1);

    if (a->type != AVMTypeInteger || a->type != b->type)
        return AVM_ERROR_WRONG_TYPE;

    ((AVMInteger)a)->value /= ((AVMInteger)b)->value;

    avm_object_free(b);
    _avm_stack_set(s,1,a);

    return avm_stack_discard(s,1);}

static AVMError _parse_Mul(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 2)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject b = avm_stack_at(s,0),
              a = avm_stack_at(s,1);

    if (a->type != AVMTypeInteger || a->type != b->type)
        return AVM_ERROR_WRONG_TYPE;

    ((AVMInteger)a)->value *= ((AVMInteger)b)->value;

    avm_object_free(b);
    _avm_stack_set(s,1,a);

    return avm_stack_discard(s,1);
}

static AVMError _parse_Dup(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject o  = avm_stack_at(s,0),
              oo = avm_object_copy(o);
    
    return avm_stack_push(s, oo);
}

static AVMError _parse_Pop(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }
    
    AVMObject o = avm_stack_pop(s);
    if (o) avm_object_free(o);

    return AVM_NO_ERROR;
}

static AVMError _parse_Def(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 2)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject value = avm_stack_at(s,0),
              key   = avm_stack_at(s,1);
    
    
    if (key->type != AVMTypeRef)
    {
        // TODO
        // assert( key->type != AVMTypeString);
        return AVM_ERROR_REF_EXPECTED;
    }

    AVMDict dict = vm->runtime.vars;
    if (dict == NULL)
    {
        dict = avm_dict_init(0);
        if (!dict)
        {
            return AVM_ERROR_NO_MEM;
        }

        vm->runtime.vars = dict;
    }
    
    AVMError err = avm_dict_set(dict, ((AVMRef)key)->ref, value);
    
    if (err != AVM_NO_ERROR)
    {
        return err;
    }

    return avm_stack_discard(s, 2);
}

static AVMError _parse_Swap(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 2)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject a = avm_stack_at(s,0),
              b = avm_stack_at(s,1);
    
    _avm_stack_set(s,0,b);
    _avm_stack_set(s,1,a);

    return AVM_NO_ERROR; 
}

static AVMError _compare(AVM vm,int* result)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 2)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject b = avm_stack_at(s,0),
              a = avm_stack_at(s,1);
    
    if (a->type != b->type)
    {
        return AVM_ERROR_DIFFERENT_TYPES;
    }

    switch((AVMType)a->type)
    {
        case AVMTypeInteger:
        {
            *result = ((AVMInteger)a)->value - ((AVMInteger)b)->value;
        }
        break;
        
        case AVMTypeString:
            *result = ((AVMString)a)->length - ((AVMString)b)->length;
            if (*result == 0)
                *result = memcmp(((AVMString)a)->data,
                                 ((AVMString)b)->data,
                                 ((AVMString)a)->length);
            break;

        default:
            return AVM_ERROR_WRONG_TYPE;

    }

    avm_stack_discard(s, 2);

    avm_object_free(b);
    avm_object_free(a);

    return AVM_NO_ERROR; 
}

#define MK_COMPARISION_FN(NAME,OP) \
static AVMError _parse_ ## NAME (AVM vm) \
{ \
    int c; \
    AVMError err = _compare(vm,&c); \
    \
    if (err != AVM_NO_ERROR) \
    { \
        return err; \
    } \
    \
    AVMInteger i = avm_create_integer(c OP 0); \
    if (!i) \
    { \
        return AVM_ERROR_NO_MEM; \
    } \
    \
    return avm_stack_push(vm->runtime.stack, (AVMObject)i); \
}

MK_COMPARISION_FN(Eq,==)
MK_COMPARISION_FN(Neq,!=)
MK_COMPARISION_FN(Lt,<)
MK_COMPARISION_FN(Lte,<=)
MK_COMPARISION_FN(Gt,>)
MK_COMPARISION_FN(Gte,>=)

static AVMError _parse_If(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 2)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject action    = avm_stack_at(s,0),
              condition = avm_stack_at(s,1);
    
    char b;
    
    if (action->type != AVMTypeCode)
        return AVM_ERROR_TYPE_NOT_EXEC;

    switch ((AVMType)condition->type)
    {
        case AVMTypeInteger:
            b = ((AVMInteger)condition)->value != 0;
            break;

        case AVMTypeString:
            b = ((AVMString)condition)->length != 0;
            break;
        default:
            return AVM_ERROR_WRONG_TYPE;
    }
    
    avm_object_free(condition);
    condition = NULL;

    avm_stack_discard(vm->runtime.stack, 2);

    if (b)
    {
        AVMError err = _run_subroutine(vm, (AVMCode)action);

        if (err != AVM_NO_ERROR)
            return err;

        avm_object_free(action);
    }

    return AVM_NO_ERROR;
}


static AVMError _parse_IfElse(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 3)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject actionB   = avm_stack_at(s,0),
              action    = avm_stack_at(s,1),
              condition = avm_stack_at(s,2);
    
    char b;
    
    if (action->type != actionB->type 
     || action->type != AVMTypeCode)
        return AVM_ERROR_TYPE_NOT_EXEC;

    switch ((AVMType)condition->type)
    {
        case AVMTypeInteger:
            b = ((AVMInteger)condition)->value != 0;
            break;

        case AVMTypeString:
            b = ((AVMString)condition)->length != 0;
            break;
        default:
            return AVM_ERROR_WRONG_TYPE;
    }
    
    avm_object_free(condition);
    condition = NULL;

    avm_stack_discard(vm->runtime.stack, 3);

    AVMError err = _run_subroutine(vm, (AVMCode) (b?action:actionB)) ;

    if (err != AVM_NO_ERROR)
        return err;

    avm_object_free(action);
    avm_object_free(actionB);

    return AVM_NO_ERROR;
}



AVMError avm_run(AVM vm, const char *code, size_t size, AVMStack s)
{
    AVMError err;
    
    if (!code || !size)
        return AVM_ERROR_NO_CODE;
    
    vm->runtime.code  = code;
    vm->runtime.pos   = 0;
    vm->runtime.size  = size;
    vm->runtime.stack = s;

    while(vm->runtime.pos < vm->runtime.size)
    {
        AVMOpcode op = vm->runtime.code[vm->runtime.pos ++];

        switch (op)
        {
            case AVMOpcodeNull:
                return AVM_ERROR_NULL_OPCODE;
        
#define OPCODE(NAME) \
            case AVMOpcode ## NAME: \
                if ( (err=_parse_##NAME(vm)) != AVM_NO_ERROR) \
                    goto failure; \
                break;
            
            OPCODE(PushInt)
            OPCODE(PushString)
            OPCODE(PushCode)
            OPCODE(PushNegInt)
            OPCODE(PushRef)
            OPCODE(PushRefVal)

            OPCODE(Pop)
            OPCODE(Swap)
            OPCODE(Dup)
            OPCODE(Add)
            OPCODE(Sub)
            OPCODE(Div)
            OPCODE(Mul)

            OPCODE(Def)

            OPCODE(Eq)
            OPCODE(Neq)
            OPCODE(Lt)
            OPCODE(Lte)
            OPCODE(Gt)
            OPCODE(Gte)

            OPCODE(If)
            OPCODE(IfElse)

            default:
                return AVM_ERROR_INVALID_OPCODE;

        }

        vm->icount ++;
    }

    return AVM_NO_ERROR;

failure:
    vm->error_code = err;
    vm->error_pos  = vm->runtime.pos;

    return err;
}
