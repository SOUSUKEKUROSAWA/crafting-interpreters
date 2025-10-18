#ifndef clox_vm_h
#define clox_vm_h

#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64 // WARNING: この数を超えるとスタックオーバーフローが発生する．
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT) // WARNING: この数を超えるとスタックオーバーフローが発生する．

/**
 * NOTE: コールフレームのアイデア
 *       どこがスタックトップになるかは，関数が呼び出されるまで分からないが，
 *       そのスタックトップからの相対位置情報はコンパイル時点で確定する．
 *       それらの相対位置によって関数の情報を表した領域を，呼び出しごとに変動するフレームと見立てて，
 *       コールフレーム（call frame）と呼ぶ．
 */
typedef struct {
    ObjClosure* closure; // 今実行しているクロージャへのポインタ．NOTE: ここから関数のチャンクにアクセス可能．
    uint8_t* ip; // 次に実行する命令へのポインタ（命令ポインタ = Instruction Pointer の略）．NOTE: 高速化のためバイトコード配列内を直接参照する．NOTE: 現在の関数自身が呼び出した関数のリターンアドレスとしても利用される．
    Value* slots; // VMの値スタックにおいて，その関数が利用できる最初のスロットへのポインタ．NOTE: 関数内のローカル変数は，このスロットからの相対位置で表され，slots[x] のようにアクセスできる．
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX]; // 進行中の関数コールの情報を保持するコールフレームの配列．NOTE: 新しいフレームほど後ろに追加されるスタックのセマンティクスを持つ．WARNING: 処理できる関数コールの深さに上限がある．
    int frameCount; // 進行中の関数コールの数．

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
