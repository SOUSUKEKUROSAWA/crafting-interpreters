#ifndef clox_value_h
#define clox_value_h

#include <string.h>
#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

#ifdef NAN_BOXING

#define QNAN ((uint64_t)0x7ffc000000000000) // クワイエットNaN: 64 bit の double（浮動小数点型の数値）のうち，全ての指数ビットが 1 のもの（= NaN），かつ仮数部の最上位ビットが 1 のもの（0 のものはシグナリング NaN と呼ばれる．）．

typedef uint64_t Value;

#define IS_NUMBER(value) (((value) & QNAN) != QNAN) // クワイエットNaN でない double 型は全て数値である．

#define AS_NUMBER(value) valueToNum(value)

#define NUMBER_VAL(num) numToValue(num)

/**
 * 所与の Value 型の値を、そのビット配列（0と1の並び）を変えずにそのまま double 型の数値として保存する．
 */
static inline double valueToNum(Value value) {
    double num;
    memcpy(&num, &value, sizeof(Value));
    return num;
}

/**
 * 所与の double 型の数値を、そのビット配列（0と1の並び）を変えずにそのまま Value 型として保存する．
 *
 * @note 通常、異なる型への代入（キャスト）を行うと、値の変換が行われる（例：intの 3 を float にすると、内部表現が変わる）．
 *       しかし、ここでは memcpy を使うことで、メモリ上の「生のデータ」をコピーする．
 */
static inline Value numToValue(double num) {
    Value value;
    memcpy(&value, &num, sizeof(double));
    return value;
}

#else

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ // サイズが大きく可変な値．データ自体はヒープに置かれ，Value のペイロードは，そのデータを指すポインタになる．（e.g. 文字列，インスタンス，関数など）
} ValueType;

/**
 * タグ付き共用体（tagged union）
 *
 * NOTE: 同じビット列を共用して，別の型として解釈することでメモリ領域を削減できる．
 * WARNING: その代わり，意図しない形でビット列の意味が解釈してしまう危険性を伴う．
 */
typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj; // サイズが大きく可変な値．データ自体はヒープに置かれ，Value のペイロードは，そのデータを指すポインタになる．（e.g. 文字列，インスタンス，関数など）
    } as;
} Value;

/**
 * AS_* マクロが誤った型解釈をしないようにするための型判定．
 *
 * WARNING: 誤った型解釈の例
 *      Value value = BOOL_VAL(true);
 *      double number = AS_NUMBER(value);
 */
#define IS_BOOL(value)      ((value).type == VAL_BOOL)
#define IS_NIL(value)       ((value).type == VAL_NIL)
#define IS_NUMBER(value)    ((value).type == VAL_NUMBER)
#define IS_OBJ(value)       ((value).type == VAL_OBJ)

#define AS_OBJ(value)       ((value).as.obj) // Value 型をアンパックしてC言語の Obj 型として値を取り出す．
#define AS_BOOL(value)      ((value).as.boolean) // Value 型をアンパックしてC言語の bool 型として値を取り出す．
#define AS_NUMBER(value)    ((value).as.number) // Value 型をアンパックしてC言語の double 型として値を取り出す．

/**
 * NOTE: 静的な型を持つ値を，動的な型を持つ clox の値に昇格させている．
 */
#define BOOL_VAL(value)     ((Value){VAL_BOOL, {.boolean = value}}) // 渡されたC言語の値から，ブール型の Value を生成する．
#define NIL_VAL             ((Value){VAL_NIL, {.number = 0}}) // 渡されたC言語の値から，nil 型の Value を生成する．
#define NUMBER_VAL(value)   ((Value){VAL_NUMBER, {.number = value}}) // 渡されたC言語の値から，数値型の Value を生成する．
#define OBJ_VAL(object)      ((Value){VAL_OBJ, {.obj = (Obj*)object}}) // 渡されたC言語の値から，オブジェクト型の Value を生成する．

#endif

typedef struct {
    int capacity; // 総容量
    int count; // 要素数（利用済みの容量）
    Value* values; // 定数プール（値の配列）の先頭要素へのポインタ
} ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);

#endif
