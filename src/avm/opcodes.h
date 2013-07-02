#ifndef OPCODES_H_INCLUDED
#define OPCODES_H_INCLUDED

typedef enum
{
    AVMOpcodeNull   = 0x00,

    AVMOpcodeRef    = 0x10,
    AVMOpcodeRefVal = 0x11,
    AVMOpcodeInt8   = 0x12,
    AVMOpcodeInt16  = 0x13,
    AVMOpcodeInt24  = 0x14,
    AVMOpcodeInt32  = 0x15,
    AVMOpcodeStr8   = 0x16,
    AVMOpcodeStr16  = 0x17,
    AVMOpcodeCode8  = 0x18,
    AVMOpcodeCode16 = 0x19,
    AVMOpcodeCode24 = 0x1a,
    AVMOpcodeCode32 = 0x1b,

    // ...
    AVMOpcodePop    = 0x1d,    
    AVMOpcodeSwap   = 0x1e,
    AVMOpcodeDup    = 0x1f,

    AVMOpcodeAdd    = 0x20,
    AVMOpcodeSub    = 0x21,
    AVMOpcodeDiv    = 0x22,
    AVMOpcodeMul    = 0x23,

    AVMOpcodeDef    = 0x30,
    
    AVMOpcodeEq     = 0x40,
    AVMOpcodeNeq    = 0x41,
    AVMOpcodeLt     = 0x42,
    AVMOpcodeLte    = 0x43,
    AVMOpcodeGt     = 0x44,
    AVMOpcodeGte    = 0x45,

    AVMOpcodeIf     = 0x50,
    AVMOpcodeIfElse = 0x51,

    AVMOpcode0      = 0x60,
    AVMOpcode1      = 0x61,
    AVMOpcode2      = 0x62,
    AVMOpcode3      = 0x63,
    AVMOpcode4      = 0x64,
    AVMOpcode5      = 0x65,
    AVMOpcode6      = 0x66,
    AVMOpcode7      = 0x67,
    AVMOpcodeN1     = 0x68,
    AVMOpcodeN2     = 0x69,
    AVMOpcodeN3     = 0x6a,
    AVMOpcodeN4     = 0x6b,
    AVMOpcodeN5     = 0x6c,
    AVMOpcodeN6     = 0x6d,
    AVMOpcodeN7     = 0x6e,

} AVMOpcode;

#endif // OPCODES_H_INCLUDED
