// NOTE: chunk = バイトコードのシーケンス
#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"

// NOTE: バイトコードは，個々の命令が 1 バイトの演算コード（operation code, opcode）
typedef enum {
    OP_RETURN,
} OpCode;

#endif
