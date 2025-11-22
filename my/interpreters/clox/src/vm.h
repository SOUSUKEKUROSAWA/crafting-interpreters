#ifndef clox_vm_h
#define clox_vm_h

#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64 // WARNING: この数を超えるとスタックオーバーフローが発生する．
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT) // WARNING: この数を超えるとスタックオーバーフローが発生する．

/**
 * @note コールフレームのアイデア：
 *  どこがスタックトップになるかは，関数が呼び出されるまで分からないが，
 *  そのスタックトップからの相対位置情報はコンパイル時点で確定する．
 *  それらの相対位置によって関数の情報を表した領域を，呼び出しごとに変動するフレームと見立てて，
 *  コールフレーム（call frame）と呼ぶ．
 */
typedef struct {
    ObjClosure* closure; // 今実行しているクロージャへのポインタ．@note ここから関数のチャンクにアクセス可能．
    uint8_t* ip; // 次に実行する命令へのポインタ（命令ポインタ = Instruction Pointer の略）．@note 高速化のためバイトコード配列内を直接参照する．@note 現在の関数自身が呼び出した関数のリターンアドレスとしても利用される．
    Value* slots; // VMの値スタックにおいて，その関数が利用できる最初のスロットへのポインタ．@note 関数内のローカル変数は，このスロットからの相対位置で表され，slots[x] のようにアクセスできる．
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX]; // 進行中の関数コールの情報を保持するコールフレームの配列．@note 新しいフレームほど後ろに追加されるスタックのセマンティクスを持つ．@warning スタックオーバーフロー回避のため，処理できる関数コールの深さに上限がある．
    int frameCount; // 進行中の関数コールの数．

    /**
     * @note スタック領域とヒープ領域について
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
     * @note インターン化とは
     *  複製排除のため，他のどの文字列とも異なることを保証すること．
     *
     *  インターン化された文字列の一覧を持つことで，ソースコード上の文字列は全て，
     *  この一覧のどれかに紐づけられるため，ポインタが同じであれば等価であると高速で判定できる．
     *
     *  オブジェクト指向言語では，メソッドコールとインスタンスフィールドの参照が，
     *  実行時に名前で検索されるので，文字列検索の速度が特に重要．
     */
    Table strings; // インターン化済みの文字列の一覧（ハッシュ表）

    ObjString* initString; // 初期化 init() のルックアップのための文字列．@note init() のルックアップ処理はインスタンス構築時に必ず呼び出されるので，高速化のためにここで定義して再利用する．

    ObjUpvalue* openUpvalues; // open upvalue の連結リストの先頭へのポインタ．ref. @note open upvalue:

    /**
     * @note 自己調整ヒープ
     *  いつGCを実行するかを決める方法の一つ．
     *  ヒープに残っている生きているオブジェクトのメモリサイズに基づいて，
     *  コレクタ自身が自動的に周期を調節する．
     */
    size_t bytesAllocated; // VMが割り当てた管理メモリの総バイト数．
    size_t nextGC; // 次回の収集のトリガとなる閾値．@note この値を，生きているオブジェクトのメモリサイズより大きくなるように調整することで，スループットとレイテンシのバランスを取る．

    Obj* objects; // 追跡用の Obj チェーン（リスト）の先頭へのポインタ

    /**
     * @note 三色抽象化
     *       white：未処理のオブジェクト（マークされていない）．
     *       gray：到達可能だが，参照する子オブジェクトの走査は未完了なオブジェクト（マークされている && グレースタックに含まれている）．
     *       black：到達可能で，参照する子オブジェクトの操作も完了しているオブジェクト（マークされている && グレースタックに含まれていない）．
     */
    int grayCount; // グレーオブジェクトの個数（動的配列内での利用済みの容量）．
    int grayCapacity; // グレーオブジェクトの動的配列の総容量
    Obj** grayStack; // グレーオブジェクトの動的配列．@note ダブルポインタ（Obj* の配列）
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
