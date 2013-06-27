#include <avm/avm.h>
#include <stdio.h>

char code[] = 
{
    0x10,0x01,0x11,0x51,0x2c,0xf8, // pushint 0x12345678
    0x11,0x84,'h','o','l','a',     // pushstr "hola"
    0x12,0x82,0x10,0x81,           // pushcode { pushint 1 }
    0x14,0x89,0xab,0xcd,0xef,      // pushref 0x89abcdef
    0x13,0x81,                     // pushnegint -1
    0x10,0x8a,                     // pushint 10
    0x20, // add
    0x10,0x83, // pushint 3
    0x21, // sub
    0x10,0x82, // pushint 2
    0x22, // div
    0x10,0x84, // 4
    0x23, // mul
    0x10,0x84, // 4
    0x01, // dup
    0x23, // mul
    0x23, // mul
    0x13,0x8c, // -0x0c
    0x21, // sub
};

int main()
{
    AVM vm     = avm_init();
    AVMStack s = avm_stack_init();
    
    AVMError e = avm_run(vm, code, sizeof(code), s);

    if (e != AVM_NO_ERROR)
    {
        printf("run failed with code %x\n", e);
        return 2;
    }
    
    uint32_t siz = avm_stack_size(s);

    printf("stack size is %u\n", siz);
    
    AVMObject o;

    while( siz-- > 0 && (o=avm_stack_pop(s))!=NULL )
    {
        AVMType t = avm_object_type(o);

        printf("elem is %u[%p]\n", t,o);
        
        switch(t)
        {
        case AVMTypeRef:
        {
            uint32_t v = avm_ref_get((AVMRef)o);
            printf("value is %x\n", v);
            break;
        }case AVMTypeInteger:
        {
            uint32_t v = avm_integer_get((AVMInteger)o);
            printf("value is %x\n", v);
            break;
        }
        case AVMTypeString:
        {
            uint32_t    len = avm_string_length((AVMString)o);
            const char *p   = avm_string_data  ((AVMString)o);

            printf("value is (%u) \"",len);
            if (len && p)
            {
                fwrite(p,len,1,stdout);
            }
            printf("\"\n");
            break;
        }
        }

    }

    return 0;
}

