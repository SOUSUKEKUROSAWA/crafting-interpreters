#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"

typedef struct {
    Chunk* chunk;

    // NOTE: 高速化のためバイトコード配列内を直接参照する．
    uint8_t* ip; // 次に実行する命令へのポインタ（命令ポインタ = Instruction Pointer の略）
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(Chunk* chunk);

#endif
