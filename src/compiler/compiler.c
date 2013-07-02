#include <stdio.h>
#include "args.h"
#include "buffer.h"
#include "parser.h"

int compile_nested(Buffer *output, FILE *input, int nestlvl)
{
    TokenType type;
    Buffer   *token = buffer_init();
    
    while (parse_input(token, &type, input) 
       && type != TokenError
       && type != TokenEOF)
    {
        fprintf(stderr, "token %u '", type);
        if (type != TokenCodeBegin && type != TokenCodeEnd)
        {
            fwrite(buffer_get_data(token), buffer_get_size(token), 1, stderr);
        }
        fprintf(stderr, "'\n");
    }

    return type != TokenError ? 0 : 9;
}

int compile(Args *args)
{
    int ret = 0;

    FILE *fin  = fopen(args->inputName, "rb"),
         *fout = fopen(args->outputName, "wb");

    if (!fin)
    {
        fprintf(stderr,"%s: Unable to open input '%s' for reading\n",
                args->exeName, args->inputName);
        return 3;
    }

    if (!fout)
    {
        fprintf(stderr,"%s: Unable to open output '%s' for writing\n",
                args->exeName, args->outputName);
        fclose(fin);
        return 4;
    }
    
    Buffer *buf = buffer_init();

    ret = compile_nested(buf, fin, 0);

    if (!ret)
    {
        size_t nbytes = buffer_get_size(buf);

        if (nbytes)
        {
            if (nbytes != fwrite(buffer_get_data(buf), nbytes, 1, fout))
            {
                fprintf(stderr,"Error writting %lu bytes to output file\n",
                        nbytes);
                return 8;
            }
        }

        fprintf(stderr,"Generated output of %lu bytes\n",
                nbytes);
    }

    fclose(fin);
    fclose(fout);

    return ret;
}


