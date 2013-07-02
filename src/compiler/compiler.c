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
    unsigned char buf[5];

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

    if     (rval >= 0 && rval <= 7)
    {
        buf[0] = AVMOpcode0 + rval;
        buffer_append(output, (const char*)buf, 1);
    }
    else if(rval >= -7 && rval <= -1)
    {
        buf[0] = AVMOpcodeN1 - rval - 1;
        buffer_append(output,(const char*) buf, 1);
    }
    else if(rval >= -128 && rval <= 127)
    {
        buf[0] = AVMOpcodeInt8;
        buf[1] = rval & 0xff;
        buffer_append(output, (const char*)buf, 2);
    }
    else if(rval >= INT16_MIN && rval <= INT16_MAX)
    {
        buf[0] = AVMOpcodeInt16;
        buf[1] = (rval>>8) & 0xff;
        buf[2] = rval & 0xff;
        buffer_append(output, (const char*)buf, 3);
    }
    else if(rval >= -(1<<23)  && rval <= (1<<23)-1)
    {
        buf[0] = AVMOpcodeInt24;
        buf[1] = 0xff & (rval>>16);
        buf[2] = 0xff & (rval>>8);
        buf[3] = 0xff & rval;
        buffer_append(output, (const char*)buf, 4);
    }
    else if(rval >= INT32_MIN && rval <= INT32_MAX)
    {
        buf[0] = AVMOpcodeInt32;
        buf[1] = 0xff & (rval>>24);
        buf[2] = 0xff & (rval>>16);
        buf[3] = 0xff & (rval>>8);
        buf[4] = 0xff & rval;
        buffer_append(output, (const char*)buf, 5);
    }
    
    return 0;
}


int compile_string(Buffer *output, Buffer *token)
{
    unsigned char buf[3];

    size_t len = buffer_get_size(token);

    if (len > UINT16_MAX)
    {
        fprintf(stderr, "String is too long!");
        return 1;
    }

    if     (len < UINT8_MAX)
    {
        buf[0] = AVMOpcodeStr8;
        buf[1] = len;
        buffer_append(output, (const char*)buf, 2);
        buffer_append(output, buffer_get_data(token), buffer_get_size(token));
    }
    else
    {
        buf[0] = AVMOpcodeStr16;
        buf[1] = 0xff & (len>>8);
        buf[2] = 0xff & len;
        buffer_append(output, (const char*)buf, 3);
        buffer_append(output, buffer_get_data(token), buffer_get_size(token));
    }
    
    return 0;
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
                rv = compile_string(output,token);
                break;

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
            if (nbytes != fwrite(buffer_get_data(buf), 1, nbytes, fout))
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


