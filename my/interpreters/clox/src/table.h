#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

/**
 * ハッシュ表の要素
 * キーと値のペア
 */
typedef struct {
    ObjString* key; // NOTE: キーは常に文字列なので，効率化のため Value（タグ付き共用体）でラップしない．
    Value value;
} Entry;

/**
 * @note count / capacity = ハッシュ表の占有率
 */
typedef struct {
    int count; // 要素数（利用済みの容量）
    int capacity; // 総容量
    Entry* entries; // エントリの配列の先頭要素へのポインタ
} Table;

void initTable(Table* table);
void freeTable(Table* table);

#endif
