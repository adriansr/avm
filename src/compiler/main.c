#include <stdio.h>
#include "args.h"
#include "compiler.h"

int main(int argc, char *argv[])
{
    Args args;
    int rv = parse_args(&args, argc, (const char**)argv);

    if (rv != 0)
        return rv;
    
    return compile(&args);
}

