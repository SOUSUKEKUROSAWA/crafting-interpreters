#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

/**
 * ハッシュ表の要素
 * キーと値のペア
 */
typedef struct {
    Value key;
    Value value;
} Entry;

/**
 * @note count / capacity = ハッシュ表の占有率
 */
typedef struct {
    int count; // 要素数（利用済みの容量）＋墓標数
    int capacity; // 総容量
    Entry* entries; // エントリの配列の先頭要素へのポインタ
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableGet(Table* table, Value key, Value* value);
bool tableSet(Table* table, Value key, Value value);
bool tableDelete(Table* table, Value key);
void tableAddAll(Table* from, Table* to);
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);

#endif
