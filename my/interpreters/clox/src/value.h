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

typedef struct {
    int capacity; // 総容量
    int count; // 要素数（利用済みの容量）
    Value* values; // 定数プール（値の配列）
} ValueArray;

void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);

#endif
