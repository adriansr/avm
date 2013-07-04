#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "args.h"
#include "buffer.h"
#include "parser.h"

#include <avm/avm.h>

#include <avm/generated/opcodes.h>
#include <avm/generated/opcode-name-table.h>

static AVM g_avm;

int compile_integer(Buffer *output, long long rval)
{
    unsigned char buf[5];

    if (rval < INT32_MIN || rval > INT32_MAX )
    {
        fprintf(stderr, "Integer value %lld doesn't fit a 32 bit integer\n",
                rval);
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

    return compile_integer(output, rval);
}
 
int compile_char(Buffer *output, Buffer *token)
{
    if (buffer_get_size(token) != 1)
    {
        fprintf(stderr, "Chars must have length 1: '%s'\n", 
                buffer_get_data(token));

        return 1;
    }

    return compile_integer(output, buffer_get_data(token)[0]);
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

int compile_op(Buffer *output, Buffer *token)
{
    char buf[1];

    const char *op = buffer_get_data(token);
    
    size_t i;
    for (i=0;OPCODE_TABLE[i].name != NULL;++i)
    {
        if (!strcasecmp(OPCODE_TABLE[i].name,op))
        {
            buf[0] = OPCODE_TABLE[i].op;
            buffer_append(output,buf,1);
            return 0;
        }
    }

    fprintf(stderr, "Invalid opcode: %s\n", op);
    return 0;
}

int compile_code(Buffer *output, Buffer *code)
{
    char buf[5];

    uint32_t len = buffer_get_size(code);

    if (len<256)
    {
        buf[0] = AVMOpcodeCode8;
        buf[1] = len & 0xff;
        buffer_append(output, buf, 2);
    }
    else if (len<65536)
    {
        buf[0] = AVMOpcodeCode16;
        buf[1] = 0xff & (len>>8);
        buf[2] = 0xff & len;
        buffer_append(output, buf, 3);

    }
    else if (len<0x1000000)
    {
        buf[0] = AVMOpcodeCode24;
        buf[1] = 0xff & (len>>16);
        buf[2] = 0xff & (len>>8);
        buf[3] = 0xff & len;
        buffer_append(output, buf, 4);
    }
    else
    {
        buf[0] = AVMOpcodeCode32;
        buf[1] = 0xff & (len>>24);
        buf[2] = 0xff & (len>>16);
        buf[3] = 0xff & (len>>8);
        buf[4] = 0xff & len;
        buffer_append(output, buf, 5);
    }

    buffer_append_buffer(output,code);
    return 0;
}

int compile_ref(Buffer *output, Buffer *token, TokenType type)
{
    char    buf[5];
    AVMHash hash;

    buf[0] = (type == TokenRef)? AVMOpcodeRef : AVMOpcodeRefVal;

    hash = avm_hash(g_avm, buffer_get_data(token), buffer_get_size(token));

    buf[1] = 0xff & (hash>>24);
    buf[2] = 0xff & (hash>>16);
    buf[3] = 0xff & (hash>>8);
    buf[4] = 0xff & (hash);
    
    buffer_append(output,buf,5);
    return 0;
}

int compile_nested(Buffer *output, FILE *input, int nestlvl)
{
    TokenType type;
    Buffer   *token = buffer_init();
    
    int rv = 0;

    while (rv==0 && parse_input(token, &type, input) 
       && type != TokenError
       && type != TokenEOF)
    {
        
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
                rv = compile_char(output, token);
                break;

            case TokenRef:
            case TokenDeref:
                rv = compile_ref(output, token, type);
                break;

            case TokenCodeBegin:
            {
                Buffer *subroutine = buffer_init();
                rv                 = compile_nested(subroutine, input, nestlvl+1);
                if (!rv)
                {
                    rv = compile_code(output, subroutine);
                }
            }
            break;

            case TokenCodeEnd:
                if (nestlvl<1)
                {
                    fprintf(stderr,"Close brace (}) found with no matching open brace\n");
                    rv = 1;
                }
                goto term;

            break;

            case TokenOp:
                rv = compile_op(output, token);
                break;

            case TokenError:
            case TokenEOF:
                /* impossible, but avoid warning */
                break;
        }
    }
term:

    return rv? rv : type != TokenError ? 0 : 9;
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
    
    g_avm = avm_init();
    if (!g_avm)
    {
        return 12;
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


