#ifndef OPCODES_H_INCLUDED
#define OPCODES_H_INCLUDED

typedef enum
{
    AVMOpcodeNull       = 0x00,
    AVMOpcodeDup        = 0x01,
    AVMOpcodePushInt    = 0x10,
    AVMOpcodePushString = 0x11,
    AVMOpcodePushCode   = 0x12,
    AVMOpcodePushNegInt = 0x13,
    AVMOpcodePushRef    = 0x14,
    // AVMOpcodePushRefVal = 0x15,
    AVMOpcodeAdd        = 0x20,
    AVMOpcodeSub        = 0x21,
    AVMOpcodeDiv        = 0x22,
    AVMOpcodeMul        = 0x23,

} AVMOpcode;

#endif // OPCODES_H_INCLUDED
