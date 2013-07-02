#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "args.h"
#include "buffer.h"
#include "parser.h"

#include <avm/opcodes.h>

int compile_number(Buffer *output, Buffer *token)
{
    long long rval;
    char     *endp;

    rval = strtoll(buffer_get_data(token), &endp, 0);

    if (*endp != '\0')
    {
        fprintf(stderr, "Invalid number '%s'\n", buffer_get_data(token));
        return 9;
    }

    if (rval < INT32_MIN || rval > INT32_MAX )
    {
        fprintf(stderr, "Integer value '%s' doesn't fit a 32 bit integer\n",
                buffer_get_data(token));
    }


}

int compile_nested(Buffer *output, FILE *input, int nestlvl)
{
    TokenType type;
    Buffer   *token = buffer_init();
    
    while (parse_input(token, &type, input) 
       && type != TokenError
       && type != TokenEOF)
    {
        int rv;
        
        buffer_zero_terminate(token);

        switch (type)
        {
            case TokenNumber:
                rv = compile_number(output,token);
                break;
                
            case TokenString:
            case TokenChar:
            case TokenRef:
            case TokenDeref:
            case TokenCodeBegin:
            case TokenCodeEnd:
            case TokenOp:
                break;
            case TokenError:
            case TokenEOF:
                /* impossible, but avoid warning */
                break;
        }
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


