#ifndef OPCODES_H_INCLUDED
#define OPCODES_H_INCLUDED

typedef enum
{
    AVMOpcodeNull       = 0x00,
    AVMOpcodePushInt    = 0x10,
    AVMOpcodePushString = 0x11,
    AVMOpcodePushCode   = 0x12,
    AVMOpcodePushNegInt = 0x13,
    AVMOpcodePushRef    = 0x14,
    AVMOpcodePushRefVal = 0x15,

    AVMOpcodePop        = 0x1d,    
    AVMOpcodeSwap       = 0x1e,
    AVMOpcodeDup        = 0x1f,
    AVMOpcodeAdd        = 0x20,
    AVMOpcodeSub        = 0x21,
    AVMOpcodeDiv        = 0x22,
    AVMOpcodeMul        = 0x23,

    AVMOpcodeDef        = 0x30,
    
    AVMOpcodeEq         = 0x40,
    AVMOpcodeNeq        = 0x41,
    AVMOpcodeLt         = 0x42,
    AVMOpcodeLte        = 0x43,
    AVMOpcodeGt         = 0x44,
    AVMOpcodeGte        = 0x45,

    AVMOpcodeIf         = 0x50,
    AVMOpcodeIfElse     = 0x51

} AVMOpcode;

#endif // OPCODES_H_INCLUDED
