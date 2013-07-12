#include <avm/avm.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

    uint32_t icount = avm_stats_icount(vm);
    
    long double t = (double)took / (double)CLOCKS_PER_SEC;
    printf("Executed %u instructions in %Lfs (%llu clocks) (%.02Lf Mips)\n",
            icount,
            t,
            (unsigned long long) took,
            ((double)icount/ (t*1000000.0))
            );

    
    avm_stack_print(s);

    if (e != AVM_NO_ERROR)
    {
        printf("\nRun failed with code %x at position %lu\n", e, avm_error_position(vm));
        //return 2;
    }

    return e;
}

