#include <avm/avm.h>
#include <stdio.h>
#include <time.h>

char code[] = 
{
    0x10,0x01,0x11,0x51,0x2c,0xf8, // pushint 0x12345678
    0x11,0x84,'h','o','l','a',     // pushstr "hola"
    0x14,0x89,0xab,0xcd,0xef,      // pushref 0x89abcdef
    0x12,0x82,0x10,0x81,           // pushcode { pushint 1 }
    0x30, // Def
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
    0x1f, // dup
    0x23, // mul
    0x23, // mul
    0x13,0x8c, // -0x0c
    0x21, // sub
    0x10,0x83, // pushint 3
    0x10,0x87, // pushint 7
    0x1e, // swap
    0x1d, // pop
    0x14, 0x00, 0x00, 0x00, 0xae,
    0x12,0x82,0x10,0x81,           // pushcode { pushint 1 }
};

int main()
{
    AVM vm     = avm_init();
    AVMStack s = avm_stack_init();
    
    clock_t start = clock();

    AVMError e = avm_run(vm, code, sizeof(code), s);

    clock_t took  = clock() - start;

    if (e != AVM_NO_ERROR)
    {
        printf("run failed with code %x\n", e);
        return 2;
    }
    
    printf("Execution took %llu clocks\n", (unsigned long long) took);

    
    avm_stack_print(s);

    return 0;
}

