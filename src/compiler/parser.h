#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#define PARSER_BUFFER_SIZE 65536

typedef enum TokenType
{
    TokenNumber,
    TokenString,
    TokenChar,
    TokenRef,
    TokenDeref,
    TokenCodeBegin,
    TokenCodeEnd,
    TokenOp,
    TokenError,
    TokenEOF
} TokenType;

char parse_input(Buffer *token, TokenType *pType, FILE *input);
                
#endif // PARSER_H_INCLUDED
