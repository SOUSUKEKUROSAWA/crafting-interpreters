#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"
#include "object.h"

/**
 * @note void* ではなく，指定した type としてポインタを返すためにマクロでラップしている．
 */
#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)

/**
 * 現在の配列を，新しいメモリ領域へコピーして拡張し，その領域のポインタを返す
 *
 * NOTE: clox における動的メモリ管理のすべてをこの関数で行うことで，
 * メモリ利用量を追跡管理しなければならないガベージコレクタを追加しやすくしている．
 *
 * | oldSize | newSize            | メモリ演算                 |
 * | ------- | ------------------ | -------------------------- |
 * | 0       | 0以外              | 新しいブロックを割り当てる |
 * | 0以外   | 0                  | 割り当てを解放する         |
 * | 0以外   | oldSize より小さい | 既存の割り当てを縮小する   |
 * | 0以外   | oldSize より大きい | 既存の割り当てを拡大する   |
 */
void* reallocate(void* pointer, size_t oldSize, size_t newSize);

void collectGarbage();

void freeObjects();

#endif
