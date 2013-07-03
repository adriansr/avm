#include <avm/avm.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

char code[] =
{
0x60, 0x61, 0x68, 0x65, 0x6C, 0x67, 0x6E, 0x12,
0x08, 0x12, 0xF8, 0x12, 0x7F, 0x12, 0x80, 0x13,
0x7F, 0xFF, 0x13, 0x80, 0x00, 0x14, 0x01, 0x00,
0x01, 0x15, 0xFF, 0x58, 0x27, 0x40, 0x15, 0x00,
0xFF, 0xFF, 0xFE, 0x15, 0x77, 0x35, 0x94, 0x00,
0x15, 0x88, 0xCA, 0x6B, 0xFF, 0x16, 0x04, 0x68,
0x6f, 0x6c, 0x61
};


char code2[] = 
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
    0x14, 0x00, 0x00, 0x00, 0xae,  // pushref 0xae
    0x12,0x82,0x10,0x80,           // pushcode { pushint 0 }
    0x30, // Def
    0x15, 0x00, 0x00, 0x00, 0xae,  // pushrefval 0xae
    0x15, 0x89, 0xab, 0xcd, 0xef,  // pushrefval 0x89abcdef
    0x15, 0x00, 0x00, 0x00, 0xae,  // pushrefval 0xae
    /*0x14,0x00,0x00,0x00,0x00,      // pushref 0
    0x12,0x85,0x15,0x00,0x00,0x00,0x00, // pushcode { pushrefval 0 }
    0x30, // def 
    0x15, 0x00, 0x00, 0x00, 0x00,  // pushrefval 0xae*/

    0x14, 0x00,0x00,0x00,0x08, // @0x8
    0x12, 0x93, // {
        0x1f, 0x10, 0x81, 0x21, 0x1f, 0x10, 0x80, 0x44, //  dup 1 sub dup 0 gt 
        0x12, 0x85, 0x15, 0x00, 0x00, 0x00, 0x08, 0x12,
        0x81, 0x1d, 0x51, // { $0x8 } { pop } ifelse } 
    0x30, // Def
    0x10, 0x03,0xff, // 10
    0x15, 0x00,0x00,0x00,0x08 // $0x8
};

static char* read_file(char *name, size_t *pSizeOut)
{
    FILE *f = fopen(name, "rb");

    *pSizeOut = 0;

    if (!f)
    {
        fprintf(stderr, "Unable to read file '%s'\n", name);
        return NULL;
    }
    
    long len;

    if (fseek(f, 0L, SEEK_END)
     || !(len=ftell(f))
     || fseek(f, 0L, SEEK_SET))
    {
        fprintf(stderr, "Unable to get file '%s' size\n", name);
        return NULL;
    }

    char *ptr = malloc(len);
    if (ptr)
    {
        if (fread(ptr,1,len,f) == len)
        {
            *pSizeOut = len;
        }
        else
        {
            fprintf(stderr, "Unable to read file '%s' contents\n", name);
            free(ptr);
            ptr = NULL;
        }

    }

    return ptr;
}

int main(int argc, char *argv[])
{
    AVM vm     = avm_init();
    AVMStack s = avm_stack_init();
    
    clock_t took;
    
    AVMError e;
    
    if (argc == 1)
    {
        clock_t start = clock();
        e = avm_run(vm, code, sizeof(code), s);
        took = clock() - start;
    }
    else
    {
        int i;
        for (i=1;i<argc;++i)
        {
            size_t len = 0;
            char  *ptr = read_file(argv[i], &len);

            if (ptr==NULL || len==0) return 2;
            
            clock_t start = clock();
            e = avm_run(vm, ptr, len, s);
            took = clock() - start;

            if (e != AVM_NO_ERROR)
                break;
        }
    }

    uint32_t icount = avm_stats_icount(vm);

    if (e != AVM_NO_ERROR)
    {
        printf("run failed with code %x\n", e);
        return 2;
    }
    
    long double t = (double)took / (double)CLOCKS_PER_SEC;
    printf("Executed %u instructions in %Lfs (%llu clocks) (%llu ips)\n",
            icount,
            t,
            (unsigned long long) took,
            (unsigned long long) ((double)icount/ t)
            );

    
    avm_stack_print(s);

    return 0;
}

