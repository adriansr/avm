#include <stdio.h>
#include "args.h"

int parse_args(Args *args, int argc, const char *argv[])
{
    int i;

    if (argc < 1) return 2;

    args->exeName    = argv[0];
    args->inputName  = NULL;
    args->outputName = NULL;

    for(i=1;i<argc;++i)
    {
        if (argv[i][0] == '-')
        {
            fprintf(stderr, "Unknown option '%s'\n",
                    argv[i]);
            return 1;
        }
        else
        {
            if (args->inputName == NULL)
                args->inputName = argv[i];
            else if (args->outputName == NULL)
                args->outputName = argv[i];
            else
            {
                fprintf(stderr,"Too many files given: '%s'\n",argv[i]);
                return 1;
            }
        }
    }

    if (args->inputName == NULL || args->outputName == NULL)
    {
        fprintf(stderr, "Usage: %s <input file> <output file>\n",
                args->exeName);
        return 1;
    }

    return 0;
}

