#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "parser.h"

static const char *BLANK_CHARS = " \t\r\n";
static const char *REF_CHARS   = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-.";

static char parse_str(Buffer *token, TokenType *pType, FILE *input, char terminator)
{
    int  c;
    char cc,
         escapeMode = 0;

    while( (c=fgetc(input)) != EOF)
    {
        cc = c;

        if (!escapeMode)
        {
            if (cc == terminator)
                break;
            if (cc == '\\')
            {
                escapeMode = 1;
                continue;
            }

            buffer_append(token, &cc, 1);
        }
        else
        {
            switch(cc)
            {
                case '\'':
                case '\"':
                case '\\':
                    break;

                default:
                    if (cc == 'r')
                        cc = '\r';
                    else if (cc == 'n')
                        cc = '\n';
                    else if (cc == 't')
                        cc = '\t';
                    else if (cc == '0')
                        cc = '\0';
                    else
                    {
                        fprintf(stderr,"Invalid escape sequence in string: \\%c\n",
                                cc);
                        *pType = TokenError;
                        return 0;
                    }
            }

            buffer_append(token, &cc, 1);
            escapeMode = 0;
        }
    }

    if (c==EOF)
    {
        *pType = TokenError;
        return 0;
    }

    return 1;
}

static char parse_ref(Buffer *token, TokenType *pType, FILE *input)
{
    int  c;
    char cc;

    while( (c=fgetc(input)) != EOF)
    {
        cc = c;

        if (!strchr(REF_CHARS, cc))
            break;

        buffer_append(token, &cc, 1);
    }

    if (c!=EOF && !strchr(BLANK_CHARS,cc))
    {
        fprintf(stderr,"Invalid character in reference name!\n");
        *pType = TokenError;
        return 0;
    }

    return 1;
}

static char parse_other(Buffer *token, TokenType *pType, FILE *input, char first)
{
    int  c;
    char cc,
         isNumber = (first>='0' && first<='9') || first=='-' || first=='+';

    buffer_append(token, &first, 1);
    
    while( (c=fgetc(input)) != EOF)
    {
        cc = c;
        
        if ( !(cc>='0' && cc<='9') )
        {
            isNumber = 0;

            if (!(cc>='A' && cc<='Z')
             && !(cc>='a' && cc<='z'))
            {
                break;
            }
        }

        buffer_append(token, &cc, 1);
    }

    if (c!=EOF && !strchr(BLANK_CHARS,cc))
    {
        fprintf(stderr,"Invalid character in operand!\n");
        *pType = TokenError;
        return 0;
    }
    
    *pType = isNumber? TokenNumber : TokenOp;
    return 1;
}

char parse_input(Buffer *token, TokenType *pType, FILE *input)
{
    int c;

    buffer_clear(token);

    do
    {
        c = fgetc(input);
    }
    while ( strchr(BLANK_CHARS, c) );

    if (c == '#')
    {
        do
        {
            c = fgetc(input);
        
            if (c == EOF)
                goto eof;
        }
        while(c != '\n');

        return parse_input(token, pType, input);
    }

    if (c == EOF)
        goto eof;

    switch(c)
    {
        case '\'': 
        {
            int rv = parse_str(token, pType, input, c);

            if (rv==1)
            {
                if (buffer_get_size(token)-=1)
                {
                    *pType = TokenChar;
                }
                else
                {
                    fprintf(stderr,"Parsing char: Must have length 1\n");
                    rv = 0;
                    *pType = TokenError;
                }
            }

            return rv;
        }
        case '\"':
        {
            *pType = TokenString;
            return parse_str(token, pType, input, c);
        }

        case '@':  
        case '$':
            *pType = c == '@'? TokenRef : TokenDeref;
            return parse_ref(token, pType, input);

        case '{':
            *pType = TokenCodeBegin;
            return 1;

        case '}':
            *pType = TokenCodeEnd;
            return 1;
    
        default:
            return parse_other(token, pType, input, c);
    }

eof:
    *pType = TokenEOF;
    return 0;
}

