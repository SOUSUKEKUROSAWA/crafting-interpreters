#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256 // WARNING: 固定値にする場合，スタックオーバーフローが発生する危険がある．

typedef struct {
    Chunk* chunk;

    // NOTE: 高速化のためバイトコード配列内を直接参照する．
    uint8_t* ip; // 次に実行する命令へのポインタ（命令ポインタ = Instruction Pointer の略）

    /**
     * NOTE: WARNING: スタック領域とヒープ領域について
     *
     * プログラムは入れ子構造になることが多く，入れ子の内側のデータほど保持期間が短くなる傾向がある．
     * つまり，最後に出てきたデータが最初に消費される，LIFO（=スタック）構造が適している．
     *
     * スタック領域は物理的なビット列がプッシュされた順序に従って連続的に並んでいるため，プッシュ／ポップ操作を高速に実行できる反面，
     * 文字列のようなサイズが大きく可変長の値を扱いずらいので，そのような値はヒープ領域に保存し，その領域へのポインタをスタックで管理する．
     */
    Value stack[STACK_MAX]; // 生成された一時的な値を追跡管理するためのスタック
    Value* stackTop; // 次にプッシュされる値の行き先（ポインタ）

    Obj* objects; // 追跡用の Obj チェーン（リスト）の先頭へのポインタ
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm; // vm を外部のモジュールに公開

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
void push(Value value);
Value pop();

#endif
