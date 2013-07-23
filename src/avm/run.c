#include "avm/internals.h"

#include <stdlib.h>
#include <string.h>

#include "avm/generated/opcodes.h"
#include "avm/generated/parsers-decl.h"
#include "avm/generated/parser-table.h"

static AVMError _parse_invalid_opcode(AVM vm)
{
    return AVM_ERROR_INVALID_OPCODE;
}

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

static AVMError _parse_Count(AVM vm)
{
    AVMStack s   = vm->runtime.stack;
    AVMInteger i = avm_create_integer(s->used);

    return i!=NULL? avm_stack_push(s,(AVMObject)i) : AVM_ERROR_NO_MEM;
}

static AVMError _parse_Index(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject opos = avm_stack_at(s,0);
    
    if (opos->type  != AVMTypeInteger)
        return AVM_ERROR_WRONG_TYPE;
 
    avm_stack_discard(s, 1);
    
    AVMObject obj = avm_stack_at(s,((AVMInteger)opos)->value);

    avm_object_free(opos);

    if (obj == NULL)
        return AVM_ERROR_STACK_RANGE;

    AVMObject oo = avm_object_copy(obj);
    return oo? avm_stack_push(s,oo) : AVM_ERROR_NO_MEM;
        
}

static AVMError _parse_Copy(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject on = avm_stack_at(s,0);
    
    if (on->type  != AVMTypeInteger)
        return AVM_ERROR_WRONG_TYPE;
 
    int32_t n = ((AVMInteger)on)->value;
    
    avm_stack_discard(s, 1);
    avm_object_free(on);
    
    if (avm_stack_size(s) < n)
        return AVM_ERROR_STACK_RANGE;

    int32_t i;
    AVMError err;

    for (i=n-1,err=AVM_NO_ERROR;err==AVM_NO_ERROR && n>0;n--)
    {
        AVMObject o   = avm_stack_at(s, i),
                  oo  = avm_object_copy(o);
        
        err = oo!=NULL? avm_stack_push(s, oo) : AVM_ERROR_NO_MEM;
    }

    return err;
}

static AVMError _parse_Roll(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 2)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject od = avm_stack_at(s,0),
              on = avm_stack_at(s,1);
    
    if (od->type  != AVMTypeInteger
     || on->type  != AVMTypeInteger)
        return AVM_ERROR_WRONG_TYPE;
 
    avm_stack_discard(s, 2);
    
    int32_t d = ((AVMInteger)od)->value,
            n = ((AVMInteger)on)->value;
    
    avm_object_free(od);
    avm_object_free(on);
    
    if (n > s->used)
        return AVM_ERROR_STACK_RANGE;
    
    if (d >= n || d <= -n)
        return AVM_ERROR_DELTA_RANGE;

    AVMObject *list = malloc( n * sizeof(AVMObject));
    if (!list)
        return AVM_ERROR_NO_MEM;

    int32_t i;
    for (i=0;i<n;++i)
    {
        list[i] = avm_stack_at(s,i);
    }

    for (i=0;i<n;++i)
    {
        int32_t pos = (i+d) % n;
        if (pos<0) pos += n;
        _avm_stack_set(s,i,list[pos]);
    }

    free(list);
    return AVM_NO_ERROR;
}

static AVMError _parse_Rev(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject on = avm_stack_at(s,0);
    
    if (on->type  != AVMTypeInteger)
        return AVM_ERROR_WRONG_TYPE;
 
    int32_t n = ((AVMInteger)on)->value;
    
    avm_stack_discard(s, 1);
    avm_object_free(on);
    
    if (avm_stack_size(s) < n)
        return AVM_ERROR_STACK_RANGE;

    int32_t i, j;

    for (i=0,j=n-1; i < j; ++i, --j)
    {
        AVMObject a = avm_stack_at(s, i),
                  b = avm_stack_at(s, j);

        _avm_stack_set(s, i, b);
        _avm_stack_set(s, j, a);
    }

    return AVM_NO_ERROR;
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
        AVMObject oo = avm_object_copy(o);
        return avm_stack_push(vm->runtime.stack, oo);
    }
    else
    {
        AVMObject saved_acc = vm->runtime.acc;
        vm->runtime.acc = 0;
        AVMError err = _run_subroutine(vm, (AVMCode)o);
        if (vm->runtime.acc)
            avm_object_free(vm->runtime.acc);
        vm->runtime.acc = saved_acc;
        return err;
    }
}

static AVMError _parse_Repeat(AVM vm)
{
   AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 2)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject action    = avm_stack_at(s,0),
              count     = avm_stack_at(s,1);
    
    if (count->type  != AVMTypeInteger
     || action->type != AVMTypeCode)
        return AVM_ERROR_WRONG_TYPE;
 
    int32_t i,
            times = ((AVMInteger)count)->value;
   
    if (times < 0)
        return AVM_ERROR_NEGATIVE_TIMES;
    
    avm_stack_discard(s, 2);

    AVMError err;
    for (i=0;i<times;++i)
    {
        err = _run_subroutine(vm, (AVMCode)action);
        if (err != AVM_NO_ERROR)
        {
            break;
        }
    }
    
    avm_object_free(action);
    avm_object_free(count);

    return err != AVM_NO_ERROR_EXIT? err : AVM_NO_ERROR;
}

static AVMError _parse_Times(AVM vm)
{
   AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 2)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject action    = avm_stack_at(s,1),
              count     = avm_stack_at(s,0);
    
    if (count->type  != AVMTypeInteger)
        return AVM_ERROR_WRONG_TYPE;
 
    int32_t i,
            times = ((AVMInteger)count)->value;
   
    if (times < 0)
        return AVM_ERROR_NEGATIVE_TIMES;

    avm_stack_discard(s, 2);

    AVMError err;

    for (i=0;i<times;++i)
    {
        AVMObject oo = avm_object_copy(action);
        if (!oo)
            return AVM_ERROR_NO_MEM;
        
        err = avm_stack_push(s,oo);
        if (err != AVM_NO_ERROR)
            break;
    }

    avm_object_free(action);
    avm_object_free(count);
    
    return err != AVM_NO_ERROR_EXIT? err : AVM_NO_ERROR;
}

static AVMError _parse_For(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 4)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject action    = avm_stack_at(s,0),
              limit     = avm_stack_at(s,1),
              increment = avm_stack_at(s,2),
              initial   = avm_stack_at(s,3);
    
    if (    limit->type != AVMTypeInteger
     || increment->type != AVMTypeInteger
     ||   initial->type != AVMTypeInteger 
     ||    action->type != AVMTypeCode)
        return AVM_ERROR_WRONG_TYPE;
    
    int32_t i   = ((AVMInteger)initial)->value,
            lim = ((AVMInteger)limit)->value,
            inc = ((AVMInteger)increment)->value;
   
    if (inc==0 // not permitted
     || (lim>i && inc<0)
     || (lim<i && inc>0))
    {
        return AVM_ERROR_BAD_INCREMENT;
    }

    avm_stack_discard(s, 4);

    AVMError err;

    for (;(inc>0 && i<=lim) || (inc<0 && i>=lim) ;i+=inc)
    {
        AVMInteger ii = avm_create_integer(i);
        err = avm_stack_push(s,(AVMObject)ii);
        if (err != AVM_NO_ERROR)
            break;

        err = _run_subroutine(vm, (AVMCode)action);

        if (err != AVM_NO_ERROR)
            break;
    }

    avm_object_free(action);
    avm_object_free(limit);
    avm_object_free(increment);
    avm_object_free(initial);
    
    return err != AVM_NO_ERROR_EXIT? err : AVM_NO_ERROR;
}

static AVMError _parse_Debug(AVM vm)
{
    return AVM_NO_ERROR;
}


static AVMError _parse_Break(AVM vm)
{
    return AVM_NO_ERROR_EXIT;
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

static AVMError _parse_Shl(AVM vm)
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
    
    uint32_t va = (uint32_t)((AVMInteger)a)->value,
             vb = (uint32_t)((AVMInteger)b)->value;

    ((AVMInteger)a)->value = (uint32_t) va << vb;
    
    avm_object_free(b);
    _avm_stack_set(s,1,a);

    return avm_stack_discard(s,1);
}

static AVMError _parse_Shr(AVM vm)
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
    
    uint32_t va = (uint32_t)((AVMInteger)a)->value,
             vb = (uint32_t)((AVMInteger)b)->value;

    ((AVMInteger)a)->value = (uint32_t) va >> vb;
    
    avm_object_free(b);
    _avm_stack_set(s,1,a);

    return avm_stack_discard(s,1);
}

static AVMError _parse_And(AVM vm)
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
    
    uint32_t va = (uint32_t)((AVMInteger)a)->value,
             vb = (uint32_t)((AVMInteger)b)->value;

    ((AVMInteger)a)->value = (uint32_t) va & vb;
    
    avm_object_free(b);
    _avm_stack_set(s,1,a);

    return avm_stack_discard(s,1);
}

static AVMError _parse_Or(AVM vm)
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
    
    uint32_t va = (uint32_t)((AVMInteger)a)->value,
             vb = (uint32_t)((AVMInteger)b)->value;

    ((AVMInteger)a)->value = (uint32_t) va | vb;
    
    avm_object_free(b);
    _avm_stack_set(s,1,a);

    return avm_stack_discard(s,1);
}

static AVMError _parse_ASet(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject b = avm_stack_at(s,0);
    
    if (vm->runtime.acc)
    {
        avm_object_free(vm->runtime.acc);
    }

    vm->runtime.acc = b;

    return avm_stack_discard(s,1);
}



static AVMError _parse_AGet(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (vm->runtime.acc == NULL)
    {
        return AVM_ERROR_ACC_NOT_SET;
    }

    AVMObject a = avm_object_copy(vm->runtime.acc);
    
    if (!a)
    {
        return AVM_ERROR_NO_MEM;
    }

    return avm_stack_push(s, a);
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

static AVMError _parse_Inc(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject a = avm_stack_at(s,0);

    if (a->type != AVMTypeInteger)
        return AVM_ERROR_WRONG_TYPE;

    ((AVMInteger)a)->value ++;
    
    return AVM_NO_ERROR;
}

static AVMError _parse_Dec(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject a = avm_stack_at(s,0);

    if (a->type != AVMTypeInteger)
        return AVM_ERROR_WRONG_TYPE;

    ((AVMInteger)a)->value --;
    
    return AVM_NO_ERROR;
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

    return avm_stack_discard(s,1);
}

static AVMError _parse_Mod(AVM vm)
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

    ((AVMInteger)a)->value %= ((AVMInteger)b)->value;

    avm_object_free(b);
    _avm_stack_set(s,1,a);

    return avm_stack_discard(s,1);
}

static AVMError _parse_EqZ(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject a = avm_stack_at(s,0);

    if (a->type != AVMTypeInteger)
        return AVM_ERROR_WRONG_TYPE;

    ((AVMInteger)a)->value =  ((AVMInteger)a)->value == 0;

    return AVM_NO_ERROR;
}

static AVMError _parse_NeqZ(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject a = avm_stack_at(s,0);

    if (a->type != AVMTypeInteger)
        return AVM_ERROR_WRONG_TYPE;

    ((AVMInteger)a)->value =  ((AVMInteger)a)->value != 0;

    return AVM_NO_ERROR;
}

static AVMError _parse_Not(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject a = avm_stack_at(s,0);

    if (a->type != AVMTypeInteger)
        return AVM_ERROR_WRONG_TYPE;

    ((AVMInteger)a)->value = ! ((AVMInteger)a)->value;

    return AVM_NO_ERROR;
}

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
    
    return oo? avm_stack_push(s, oo) : AVM_ERROR_NO_MEM;
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


static AVMError _parse_Undef(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject key   = avm_stack_at(s,0);
    
    
    if (key->type != AVMTypeRef)
    {
        // TODO
        // assert( key->type != AVMTypeString);
        return AVM_ERROR_REF_EXPECTED;
    }

    AVMDict dict = vm->runtime.vars;
    if (dict != NULL)
    {
        AVMError err = avm_dict_remove(dict, ((AVMRef)key)->ref);
    
        if (err != AVM_NO_ERROR)
        {
            return err;
        }

        return avm_stack_discard(s, 2);
    }

    return AVM_NO_ERROR;
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

static AVMError _parse_IsMark(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject  a = avm_stack_at(s,0);
    AVMInteger b = avm_create_integer(a->type == AVMTypeMark);
    
    if (b)
    {
        avm_object_free(a);
        avm_stack_discard(s,1);
    }

    return avm_stack_push(s, (AVMObject)b);
}

static AVMError _parse_CTM(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    uint32_t n = avm_stack_size(s);
    if (n < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    
    uint32_t i;
    for (i=0;i<n;++i)
    {
        AVMObject a = avm_stack_at(s,i);
        if (a->type == AVMTypeMark)
            break;
    }
    
    if (i<n)
    {
        AVMInteger b = avm_create_integer(i);
        return avm_stack_push(s, (AVMObject)b);
    }

    return AVM_ERROR_MARK_NOT_FOUND;
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

    AVMError err = AVM_NO_ERROR;

    if (b)
    {
        err = _run_subroutine(vm, (AVMCode)action);

        avm_object_free(action);
    }

    return err;
}

static AVMError _parse_At(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 2)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject string   = avm_stack_at(s,1),
              position = avm_stack_at(s,0);
    
    
    if (string->type != AVMTypeString || position->type != AVMTypeInteger)
        return AVM_ERROR_WRONG_TYPE;
    
    int32_t pos = ((AVMInteger)position)->value;
    int32_t len = ((AVMString)string)->length;

    if (pos>=0)
    {
        if (pos >= len)
            return AVM_ERROR_RANGE_CHECK;
    }
    else
    {
        if (pos < -len)
            return AVM_ERROR_RANGE_CHECK;
        
        pos = len + pos;
    }

    unsigned char value = (unsigned char) ((AVMString)string)->data[pos];
    
    ((AVMInteger)position)->value = value;
    
    return AVM_NO_ERROR;
}

static AVMError _parse_Head(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject string   = avm_stack_at(s,0);
    
    if (string->type != AVMTypeString)
        return AVM_ERROR_WRONG_TYPE;
    
    AVMInteger o = avm_create_integer(-1);
    if (o == NULL) return AVM_ERROR_NO_MEM;

    if ( ((AVMString)string)->length > 0)
    {
        ((AVMString)string)->length --;

        o->value = ((AVMString)string)->data[0];

        memmove(((AVMString)string)->data,
                ((AVMString)string)->data+1,
                ((AVMString)string)->length);
    }
    else
    {
        avm_stack_discard(s,1);
        avm_object_free(string);
    }
    
    return avm_stack_push(s, (AVMObject)o);
}


static AVMError _parse_Tail(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject string   = avm_stack_at(s,0);
    
    if (string->type != AVMTypeString)
        return AVM_ERROR_WRONG_TYPE;
    
    AVMInteger o = avm_create_integer(-1);
    if (o == NULL) return AVM_ERROR_NO_MEM;

    if ( ((AVMString)string)->length > 0)
    {
        ((AVMString)string)->length --;

        o->value = ((AVMString)string)->data[((AVMString)string)->length];
    }
    else
    {
        avm_stack_discard(s,1);
        avm_object_free(string);
    }
    
    return avm_stack_push(s, (AVMObject)o);
}

static AVMError _parse_Len(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject string   = avm_stack_at(s,0);
    
    if (string->type != AVMTypeString)
        return AVM_ERROR_WRONG_TYPE;
    
    uint32_t len = ((AVMString)string)->length;

    AVMInteger o = avm_create_integer(len);
    if (o == NULL) return AVM_ERROR_NO_MEM;
    return avm_stack_push(vm->runtime.stack, (AVMObject)o);
}

static AVMError _parse_Expl(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMObject string = avm_stack_at(s,0);
    
    if (string->type != AVMTypeString)
        return AVM_ERROR_WRONG_TYPE;
    
    uint32_t len = ((AVMString)string)->length;
    
    AVMError err = AVM_NO_ERROR;
    
    avm_stack_discard(s, 1);

    while (err==AVM_NO_ERROR && len>0)
    {
        --len;

        AVMInteger i = avm_create_integer( ((AVMString)string)->data[len] );
        err          = i ? avm_stack_push(vm->runtime.stack, (AVMObject)i)
                         : AVM_ERROR_NO_MEM;
    }

    avm_object_free(string);

    return err;
}

static AVMError _parse_Mark(AVM vm)
{
    AVMStack s = vm->runtime.stack;
    
    AVMMark mark = avm_create_mark(0);
    
    return mark?avm_stack_push(s, (AVMObject)mark) : AVM_ERROR_NO_MEM;
}

static AVMError _parse_Split(AVM vm)
{
    /*
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 2)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMInteger pos = (AVMInteger)avm_stack_at(s,1);
    AVMString  str = (AVMString)avm_stack_at(s,0);
    
    if (pos->type != AVMTypeInteger || str->type != AVMTypeString)
        return AVM_ERROR_WRONG_TYPE;
    
    int32_t p   = pos->value,
            len = (int32_t)str->length;
    
    if (p>=len || p<=-len)
        return AVM_ERROR_STRING_RANGE;
    
    if (p<0) p = n-p;

    AVMString part = avm_create_string(a->data, ->length);
    if (r==NULL)
        return AVM_ERROR_NO_MEM;

    if (a->length)
    {
        memcpy(&r->data[0], a->data, a->length);
    }

    if (b->length)
    {
        memcpy(&r->data[a->length], b->data, b->length);
    }

    avm_stack_discard(s,1);
    avm_object_free((AVMObject)a);
    avm_object_free((AVMObject)b);
    
    _avm_stack_set(s,0,(AVMObject)r);
    return AVM_NO_ERROR;
    */
    return AVM_ERROR_UNIMPLEMENTED;
}


static AVMError _parse_Join(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 2)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMString a = (AVMString)avm_stack_at(s,1),
              b = (AVMString)avm_stack_at(s,0);
    
    if (a->type != AVMTypeString || b->type != AVMTypeString)
        return AVM_ERROR_WRONG_TYPE;

    AVMString r = avm_create_string_empty(a->length + b->length);
    if (r==NULL)
        return AVM_ERROR_NO_MEM;

    if (a->length)
    {
        memcpy(&r->data[0], a->data, a->length);
    }

    if (b->length)
    {
        memcpy(&r->data[a->length], b->data, b->length);
    }

    avm_stack_discard(s,1);
    avm_object_free((AVMObject)a);
    avm_object_free((AVMObject)b);
    
    _avm_stack_set(s,0,(AVMObject)r);
    return AVM_NO_ERROR;
}

static AVMError _parse_Impl(AVM vm)
{
    AVMStack s = vm->runtime.stack;

    if (avm_stack_size(s) < 1)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMInteger num = (AVMInteger)avm_stack_at(s,0);
    
    if (num->type != AVMTypeInteger)
        return AVM_ERROR_WRONG_TYPE;
    
    uint32_t i,n = num->value;
    
    AVMError err = AVM_NO_ERROR;
    
    avm_stack_discard(s, 1);
    avm_object_free((AVMObject)num);

    if (avm_stack_size(s) < n)
    {
        return AVM_ERROR_NOT_ENOUGH_ARGS;
    }

    AVMString str = avm_create_string_empty(n);

    for (i=0;i<n;++i)
    {
        AVMInteger ii = (AVMInteger)avm_stack_at(s,i);
        if (ii && ii->type == AVMTypeInteger)
        {
            uint32_t v = ii->value;
            avm_object_free((AVMObject)ii);

            if (v < 256)
            {
                str->data[i] = v;
            }
            else
            {
                err = AVM_ERROR_CHAR_VALUE;
                break;
            }

        }
        else
        {
            err = AVM_ERROR_WRONG_TYPE;
            break;
        }
    }
    avm_stack_discard(s,n);
    avm_stack_push(s, (AVMObject)str);
    return err;
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

    avm_object_free(action);
    avm_object_free(actionB);

    return err;
}

static AVMError _parse_Null(AVM vm)
{
    return AVM_ERROR_NULL_OPCODE;
}

AVMError avm_run(AVM vm, const char *code, size_t size, AVMStack s)
{
    AVMError err;
    
    if (!code || !size)
        return AVM_NO_ERROR; // We want empty codeblocks {} to work
        //return AVM_ERROR_NO_CODE;
    
    vm->runtime.code  = code;
    vm->runtime.pos   = 0;
    vm->runtime.size  = size;
    vm->runtime.stack = s;

    while(vm->runtime.pos < vm->runtime.size)
    {
        AVMOpcode op = (unsigned char)vm->runtime.code[vm->runtime.pos++];

        err = PARSER_TABLE[op](vm);
        
        if (err != AVM_NO_ERROR)
            goto failure;

        vm->icount ++;
    }

    return AVM_NO_ERROR;

failure:
    vm->error_code = err;
    vm->error_pos  = vm->runtime.pos;

    return err;
}
