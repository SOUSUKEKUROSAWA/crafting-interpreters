#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75 // ハッシュ表の最大許容占有率（衝突を避けるために 1 未満で調整する）

void initTable(Table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable(Table* table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

/**
 * 所与のキーから計算したインデックスに一致するエントリを返す．
 *
 * @note そのエントリが空だったとしてもそれをそのまま返す（空ということは，その場所に新しいエントリを記入できることを示す）．
 */
static Entry* findEntry(
    Entry* entries,
    int capacity,
    ObjString* key
) {
    /**
     * NOTE: ハッシュ表が高速かつコンパクトな理由
     *  ハッシュを総容量で割った余りをインデックスとして配列に格納されているから．
     *  キーとインデックスを対応付けることで，先頭からの探索ではなく，インデックスによる直アクセスが可能になる（よって高速）．
     *  キーの文字列を直接インデックスに使うのではなく，そのハッシュを使うことで，キーの文字列の長さに関わらず一定文字数でインデックスを管理できる．
     *  かつ，そのハッシュを総容量で割った余りをインデックスに使うことで，ハッシュ空間全てに対応する巨大な配列を作らなくて済む（よってコンパクト）．
     *
     * WARNING: 総容量は小さすぎると，衝突のリスクが高まるので，占有率を調整して，適切に拡張する必要がある．
     */
    uint32_t index = key->hash % capacity;

    /**
     * WARNING: 占有率が 100% にならないように capacity が調整されることを前提としている．
     *  占有率が 100% になると無限ループになってしまう．
     */
    for (;;) {
        Entry* entry = &entries[index];

        if (entry->key == key || entry->key == NULL) {
            return entry;
        }

        /**
         * 線形探針（linear probing）
         * 異なるキーが存在していた場合（＝衝突を検知した場合）は隣を順に探索していく．
         * 総容量より大きかったら先頭に戻る．
         */
        index = (index + 1) % capacity;
    }
}

/**
 * ハッシュ表にエントリを追加／上書きする．
 *
 * @return true: 新規エントリーの追加, false: 既存のエントリーの上書き
 */
bool tableSet(Table* table, ObjString* key, Value value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;
    if (isNewKey) table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}
