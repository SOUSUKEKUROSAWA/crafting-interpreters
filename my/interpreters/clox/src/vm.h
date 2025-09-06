#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "table.h"
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

    Table globals; // グローバル変数の一覧（ハッシュ表）

    /**
     * NOTE: インターン化とは
     *  複製排除のため，他のどの文字列とも異なることを保証すること．
     *
     *  インターン化された文字列の一覧を持つことで，ソースコード上の文字列は全て，
     *  この一覧のどれかに紐づけられるため，ポインタが同じであれば等価であると高速で判定できる．
     *
     *  オブジェクト指向言語では，メソッドコールとインスタンスフィールドの参照が，
     *  実行時に名前で検索されるので，文字列検索の速度が特に重要．
     */
    Table strings; // インターン化済みの文字列の一覧（ハッシュ表）

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
