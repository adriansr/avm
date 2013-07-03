#include "avm/internals.h"
#include "avm/opcodes.h"

#include <stdlib.h>
#include <string.h>

#define MK_NUMBER_FN(N) \
static AVMError _parse_ ## N(AVM vm) \
{ \
    AVMInteger o = avm_create_integer(N); \
    if (o == NULL) return AVM_ERROR_NO_MEM; \
    return avm_stack_push(vm->runtime.stack, (AVMObject)o); \
}

MK_NUMBER_FN(0)
MK_NUMBER_FN(1)
MK_NUMBER_FN(2)
MK_NUMBER_FN(3)
MK_NUMBER_FN(4)
MK_NUMBER_FN(5)
MK_NUMBER_FN(6)
MK_NUMBER_FN(7)

#define MK_NEG_NUMBER_FN(N) \
static AVMError _parse_N ## N(AVM vm) \
{ \
    AVMInteger o = avm_create_integer(-N); \
    if (o == NULL) return AVM_ERROR_NO_MEM; \
    return avm_stack_push(vm->runtime.stack, (AVMObject)o); \
}

MK_NEG_NUMBER_FN(1)
MK_NEG_NUMBER_FN(2)
MK_NEG_NUMBER_FN(3)
MK_NEG_NUMBER_FN(4)
MK_NEG_NUMBER_FN(5)
MK_NEG_NUMBER_FN(6)
MK_NEG_NUMBER_FN(7)

static AVMError _read_uint8(AVM vm, uint32_t *value)
{
    if (vm->runtime.pos + 1 > vm->runtime.size)
    {
        return AVM_ERROR_REF_TRUNCATED;
    }
    
    uint8_t *p = (uint8_t*)&vm->runtime.code[vm->runtime.pos];

    *value = p[0];
    
    vm->runtime.pos += 1;

    return AVM_NO_ERROR;
}

static AVMError _read_uint16(AVM vm, uint32_t *value)
{
    if (vm->runtime.pos + 2 > vm->runtime.size)
    {
        return AVM_ERROR_REF_TRUNCATED;
    }
    
    uint8_t *p = (uint8_t*)&vm->runtime.code[vm->runtime.pos];

    *value = (p[0]<<8) | p[1];
    
    vm->runtime.pos += 2;
    return AVM_NO_ERROR;
}

static AVMError _read_uint24(AVM vm, uint32_t *value)
{
    if (vm->runtime.pos + 3 > vm->runtime.size)
    {
        return AVM_ERROR_REF_TRUNCATED;
    }
    
    uint8_t *p = (uint8_t*)&vm->runtime.code[vm->runtime.pos];

    *value = p[2] | (p[1]<<8) | (p[0]<<16);
    
    vm->runtime.pos += 3;
    return AVM_NO_ERROR;
}

static AVMError _read_uint32(AVM vm, uint32_t *value)
{
    if (vm->runtime.pos + 4 > vm->runtime.size)
    {
        return AVM_ERROR_REF_TRUNCATED;
    }
    
    uint8_t *p = (uint8_t*)&vm->runtime.code[vm->runtime.pos];

    *value = p[3] | (p[2]<<8) | (p[1]<<16) | (p[0]<<24);
    
    vm->runtime.pos += 4;
    return AVM_NO_ERROR;
}

static AVMError _read_int8(AVM vm, int32_t *value)
{
    if (vm->runtime.pos + 1 > vm->runtime.size)
    {
        return AVM_ERROR_REF_TRUNCATED;
    }
    
    int8_t *p = (int8_t*)&vm->runtime.code[vm->runtime.pos];

    *value = p[0];
    
    vm->runtime.pos += 1;

    return AVM_NO_ERROR;
}

static AVMError _read_int16(AVM vm, int32_t *value)
{
    int16_t vvalue;

    if (vm->runtime.pos + 2 > vm->runtime.size)
    {
        return AVM_ERROR_REF_TRUNCATED;
    }
    
    uint8_t *p = (uint8_t*)&vm->runtime.code[vm->runtime.pos];

    vvalue = (p[0]<<8) | p[1];
    
    vm->runtime.pos += 2;
    
    *value = vvalue;
    return AVM_NO_ERROR;
}

static AVMError _read_int24(AVM vm, int32_t *value)
{
    uint32_t vvalue;
    
    if (vm->runtime.pos + 3 > vm->runtime.size)
    {
        return AVM_ERROR_REF_TRUNCATED;
    }
    
    uint8_t *p = (uint8_t*)&vm->runtime.code[vm->runtime.pos];

    vvalue = p[2] | (p[1]<<8) | (p[0]<<16);
    
    vm->runtime.pos += 3;
    
    /* sign extend to 32 bits */
    if (vvalue & 0x800000)
    {
        vvalue |= 0xff000000;
    }
    
    *value = vvalue;
    return AVM_NO_ERROR;
}

static AVMError _read_int32(AVM vm, int32_t *value)
{
    if (vm->runtime.pos + 4 > vm->runtime.size)
    {
        return AVM_ERROR_REF_TRUNCATED;
    }
    
    uint8_t *p = (uint8_t*)&vm->runtime.code[vm->runtime.pos];

    *value = p[3] | (p[2]<<8) | (p[1]<<16) | (p[0]<<24);
    
    vm->runtime.pos += 4;
    return AVM_NO_ERROR;
}

#define MK_NUMBER_BITS_FN(BITS) \
static AVMError _parse_Int ## BITS(AVM vm) \
{ \
    int32_t value; \
    AVMError err = _read_int##BITS(vm, &value); \
    if (err != AVM_NO_ERROR) \
        return err; \
    AVMInteger o = avm_create_integer(value); \
    if (o == NULL) \
        return AVM_ERROR_NO_MEM; \
    return avm_stack_push(vm->runtime.stack, (AVMObject)o); \
}

MK_NUMBER_BITS_FN(8)
MK_NUMBER_BITS_FN(16)
MK_NUMBER_BITS_FN(24)
MK_NUMBER_BITS_FN(32)

static AVMError _parse_Ref(AVM vm)
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

static AVMError _parse_RefVal(AVM vm)
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

#define MK_STR_BITS_FN(BITS) \
static AVMError _parse_Str##BITS(AVM vm) \
{ \
    uint32_t length; \
    AVMError err = _read_uint##BITS(vm,&length); \
    if (err != AVM_NO_ERROR) \
        return AVM_ERROR_REF_TRUNCATED; \
    if (vm->runtime.pos + length > vm->runtime.size) \
        return AVM_ERROR_STR_TRUNCATED; \
    AVMString o = avm_create_string(&vm->runtime.code[vm->runtime.pos], \
                                    length); \
    if (o == NULL) \
        return AVM_ERROR_NO_MEM; \
    vm->runtime.pos += length; \
    return avm_stack_push(vm->runtime.stack, (AVMObject)o); \
}

MK_STR_BITS_FN(8)
MK_STR_BITS_FN(16)

#define MK_CODE_BITS_FN(BITS) \
static AVMError _parse_Code##BITS (AVM vm) \
{ \
    uint32_t length; \
    AVMError err = _read_uint##BITS(vm,&length); \
    if (err != AVM_NO_ERROR) \
        return err; \
    if (vm->runtime.pos + length > vm->runtime.size) \
        return AVM_ERROR_CODE_TRUNCATED; \
    AVMCode o = avm_create_code(&vm->runtime.code[vm->runtime.pos], \
                                length); \
    if (o == NULL) \
        return AVM_ERROR_NO_MEM; \
    vm->runtime.pos += length; \
    return avm_stack_push(vm->runtime.stack, (AVMObject)o); \
}

MK_CODE_BITS_FN(8)
MK_CODE_BITS_FN(16)
MK_CODE_BITS_FN(24)
MK_CODE_BITS_FN(32)

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
            
            OPCODE(Ref)
            OPCODE(RefVal)
            OPCODE(Int8)
            OPCODE(Int16)
            OPCODE(Int24)
            OPCODE(Int32)
            OPCODE(Str8)
            OPCODE(Str16)
            OPCODE(Code8)
            OPCODE(Code16)
            OPCODE(Code24)
            OPCODE(Code32)

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

            OPCODE(0)
            OPCODE(1)
            OPCODE(2)
            OPCODE(3)
            OPCODE(4)
            OPCODE(5)
            OPCODE(6)
            OPCODE(7)
            
            OPCODE(N1)
            OPCODE(N2)
            OPCODE(N3)
            OPCODE(N4)
            OPCODE(N5)
            OPCODE(N6)
            OPCODE(N7)

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
