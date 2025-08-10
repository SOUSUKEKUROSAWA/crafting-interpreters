#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
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

#define AS_BOOL(value)      ((value).as.boolean) // Value 型をアンパックしてC言語の bool 型として値を取り出す．
#define AS_NUMBER(value)    ((value).as.number) // Value 型をアンパックしてC言語の double 型として値を取り出す．

/**
 * NOTE: 静的な型を持つ値を，動的な型を持つ clox の値に昇格させている．
 */
#define BOOL_VAL(value)     ((Value){VAL_BOOL, {.boolean = value}}) // 渡されたC言語の値から，ブール型の Value を生成する．
#define NIL_VAL             ((Value){VAL_NIL, {.number = 0}}) // 渡されたC言語の値から，nil 型の Value を生成する．
#define NUMBER_VAL(value)   ((Value){VAL_NUMBER, {.number = value}}) // 渡されたC言語の値から，数値型の Value を生成する．

typedef struct {
    int capacity; // 総容量
    int count; // 要素数（利用済みの容量）
    Value* values; // 定数プール（値の配列）
} ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);

#endif
