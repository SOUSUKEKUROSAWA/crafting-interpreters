#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256 // WARNING: 固定値にする場合，スタックオーバーフローが発生する危険がある．

typedef struct {
    Chunk* chunk;

    // NOTE: 高速化のためバイトコード配列内を直接参照する．
    uint8_t* ip; // 次に実行する命令へのポインタ（命令ポインタ = Instruction Pointer の略）

    Value stack[STACK_MAX]; // 生成された一時的な値を追跡管理するためのスタック
    Value* stackTop; // 次にプッシュされる値の行き先（ポインタ）
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(Chunk* chunk);
void push(Value value);
Value pop();

#endif
