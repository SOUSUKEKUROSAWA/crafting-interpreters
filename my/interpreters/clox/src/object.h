#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "table.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value) isObjType(value, OBJ_CLASS)
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value) isObjType(value, OBJ_INSTANCE)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)
#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))
#define AS_CLASS(value) ((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
#define AS_INSTANCE(value) ((ObjInstance*)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
    OBJ_BOUND_METHOD, // インスタンスメソッド
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_NATIVE, // 言語組み込み関数
    OBJ_STRING,
    OBJ_UPVALUE,
} ObjType;

/**
 * すべてのオブジェクト型に共通する状態
 *
 * オブジェクト型:
 *      サイズが大きく可変な値．データ自体はヒープに置かれ，Value のペイロードは，
 *      そのデータを指すポインタになる．（e.g. 文字列，インスタンス，関数など）
 */
struct Obj {
    ObjType type;
    bool isMarked; // GCがリサイクルするオブジェクトを判定するためのフラグ．GC実行時に到達可能であれば true がセットされ，false のものは到達不可能と見なされリサイクルされる．
    struct Obj* next; // 追跡用の Obj チェーン（リスト）における，次の Obj へのポインタ
};

typedef struct {
    Obj obj; // オブジェクト型共通のデータ．@note 構造体継承：このフィールドを先頭に持ってくることで，ObjFunction* を Obj* に安全にキャストできる（先頭が完全に一致するため）．
    int arity; // その関数が受け取りたいパラメータの数
    int upvalueCount; // その関数がキャプチャした上位値の個数
    Chunk chunk; // 関数の本文を格納するチャンク
    ObjString* name; // 関数名（ランタイムエラー時などに利用する）
} ObjFunction;

/**
 * @param argCount 引数の個数
 * @param args スタック上の最初の引き数へのポインタ
 */
typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
    Obj obj; // オブジェクト型共通のデータ．ref. 構造体継承
    NativeFn function; // ネイティブ関数（言語組み込みの関数）へのポインタ．
} ObjNative;

struct ObjString {
    Obj obj; // オブジェクト型共通のデータ．ref. 構造体継承
    int length; // 割り当てられたバイト数
    char* chars; // 文字配列の先頭へのポインタ
    uint32_t hash; // その文字列に対応するハッシュ（ハッシュ再計算を不要にするためにキャッシュとして保持）
};

typedef struct ObjUpvalue {
    Obj obj; // オブジェクト型共通のデータ．ref. 構造体継承
    Value* location; // 閉じ込めた（クロージャ・キャプチャした）変数へのポインタ
    Value closed; // closed upvalue の置き場所となるヒープ領域．スタックにあったローカル変数が解放時にここに移されることで，任意のクロージャがスタック解放後もキャプチャしたローカル変数にアクセスできる．
    struct ObjUpvalue* next; // スタックのより後方（下方）にあるローカル変数を参照する「次の open upvalue」へのポインタ．ref. @note open upvalue:
} ObjUpvalue;

/**
 * 関数実行時に ObjFunction をラップして，
 * 実行時の変数なども閉じ込めて（Closure して）おく構造体．
 */
typedef struct {
    Obj obj; // オブジェクト型共通のデータ．ref. 構造体継承
    ObjFunction* function;
    ObjUpvalue** upvalues; // このクロージャがキャプチャしている上位値ポインタの配列．@note ダブルポインタ
    int upvalueCount; // このクロージャが保持する上位値の数．@note ObjFunction も upvalueCount を保持しているので，本来は不要だが，GCが ObjClousure の上位値配列サイズを知りたい場合があるので，あえて冗長性を持たせている．
} ObjClosure;

typedef struct {
    Obj obj; // オブジェクト型共通のデータ．ref. 構造体継承
    ObjString* name; // クラス名．実行時のスタックトレースなどで利用される．
    Table methods; // クラスが所有するメソッド群のハッシュテーブル．
} ObjClass;

typedef struct {
    Obj obj; // オブジェクト型共通のデータ．ref. 構造体継承
    ObjClass* klass; // 自身が所属するクラスへのポインタ
    Table fields; // インスタンス固有のフィールドを保持するハッシュテーブル．@note Lox ユーザーは実行時にインスタンスにフィールドを追加できるので，拡張可能である必要があり，なおかつフィールドの検索を高速で行いたいのでハッシュテーブルを用いる．
} ObjInstance;

/**
 * 束縛メソッド
 * 呼び出し元のインスタンスの状態をメソッドに紐づけるための構造体．
 *
 * @note Lox では，メソッドへのアクセスとコールが同時に行われない可能性があるため，
 *       あとで呼び出されるまでアクセス元のインスタンスを覚えておく必要がある．
 */
typedef struct {
    Obj obj; // オブジェクト型共通のデータ．ref. 構造体継承
    Value receiver; // アクセス元のインスタンス．@note ObjInstance ではない，汎用的な関数も渡せるように Value で定義している．
    ObjClosure* method; // 取り扱うメソッド（クロージャ）へのポインタ．
} ObjBoundMethod;

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method);
ObjClass* newClass(ObjString* name);
ObjClosure* newClosure(ObjFunction* function);
ObjFunction* newFunction();
ObjInstance* newInstance(ObjClass* klass);
ObjNative* newNative(NativeFn function);
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
ObjUpvalue* newUpvalue(Value* slot);

void printObject(Value value);

/**
 * @note inline: 関数呼び出しの代わりに「呼び出し箇所へ関数本体を展開してよい」というコンパイラへの最適化ヒント（必ず展開される保証はない）
 * @warning 引数 value を2度利用しているため，副作用が発生しないようにマクロに直接展開せずに関数化している．
 *          e.g. IS_STRING(POP()) のように書いた場合，マクロに直接展開すると，POPが2度行われてしまう．
 */
static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
