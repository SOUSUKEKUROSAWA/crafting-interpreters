#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) ((ObjString*)AS_OBJ(value)->chars)

typedef enum {
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
};

struct ObjString {
    Obj obj; // オブジェクト型共通のデータ．NOTE: このフィールドを先頭に持ってくることで，ObjString* を Obj* に安全にキャストできる（先頭が完全に一致するため）．
    int length; // 割り当てられたバイト数
    char* chars; // 文字配列へのポインタ
};

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
