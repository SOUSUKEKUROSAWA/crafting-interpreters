#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)
#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)
#define AS_NATIVE_ARITY(value) (((ObjNative*)AS_OBJ(value))->arity)
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_STRING,
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
    struct Obj* next; // 追跡用の Obj チェーン（リスト）における，次の Obj へのポインタ
};

typedef struct {
    Obj obj; // オブジェクト型共通のデータ．NOTE: 構造体継承：このフィールドを先頭に持ってくることで，ObjFunction* を Obj* に安全にキャストできる（先頭が完全に一致するため）．
    int arity; // その関数が受け取りたいパラメータの数
    Chunk chunk; // 関数の本文を格納するチャンク
    ObjString* name; // 関数名（ランタイムエラー時などに利用する）
} ObjFunction;

/**
 * @param argCount 引数の個数
 * @param args スタック上の最初の引き数へのポインタ
 */
typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
    Obj obj; // オブジェクト型共通のデータ．NOTE: 構造体継承：このフィールドを先頭に持ってくることで，ObjFunction* を Obj* に安全にキャストできる（先頭が完全に一致するため）．
    NativeFn function; // ネイティブ関数（言語組み込みの関数）へのポインタ．
    int arity; // そのネイティブ関数が受け取る引数の数
} ObjNative;

struct ObjString {
    Obj obj; // オブジェクト型共通のデータ．NOTE: 構造体継承：このフィールドを先頭に持ってくることで，ObjString* を Obj* に安全にキャストできる（先頭が完全に一致するため）．
    int length; // 割り当てられたバイト数
    char* chars; // 文字配列の先頭へのポインタ
    uint32_t hash; // その文字列に対応するハッシュ（ハッシュ再計算を不要にするためにキャッシュとして保持）
};

ObjFunction* newFunction();
ObjNative* newNative(NativeFn function, int arity);
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);

void printObject(Value value);

/**
 * NOTE: inline: 関数呼び出しの代わりに「呼び出し箇所へ関数本体を展開してよい」というコンパイラへの最適化ヒント（必ず展開される保証はない）
 * WARNING: 引数 value を2度利用しているため，副作用が発生しないようにマクロに直接展開せずに関数化している．
 *          e.g. IS_STRING(POP()) のように書いた場合，マクロに直接展開すると，POPが2度行われてしまう．
 */
static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
