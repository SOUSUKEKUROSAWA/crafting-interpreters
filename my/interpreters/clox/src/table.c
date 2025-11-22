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
    uint32_t index = key->hash & (capacity - 1); // note: このハッシュ表のサイズは必ず2のN乗になるので，key->hash % capacity と結果は同じだが，高速化のためにビット演算で実装している．
    Entry* tombstone = NULL; // 最初に見つけた墓標の位置（ポインタ）

    /**
     * WARNING: 占有率が 100% にならないように capacity が調整されることを前提としている．
     *  占有率が 100% になると無限ループになってしまう．
     *
     * WARNING: 墓標エントリだけ（空エントリ無し）になってしまっても無限ループになってしまう．
     *  だから，墓標エントリは記入済みとして扱う．
     */
    for (;;) {
        Entry* entry = &entries[index];

        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                // 空エントリの場合，
                // 墓標を通過していたら，その墓標を返して再利用してもらう．
                return tombstone != NULL ? tombstone : entry;
            } else {
                // 墓標エントリの場合，
                // それが最初に見つけた墓標であれば記録する．
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (entry->key == key) {
            return entry;
        }

        /**
         * 線形探針（linear probing）
         * 異なるキーが存在していた場合（＝衝突を検知した場合）は隣を順に探索していく．
         * 総容量より大きかったら先頭に戻る．
         */
        index = (index + 1) & (capacity - 1); // note: このハッシュ表のサイズは必ず2のN乗になるので，(index + 1) % capacity と結果は同じだが，高速化のためにビット演算で実装している．
    }
}

bool tableGet(Table* table, ObjString* key, Value* value) {
    if (table->count == 0) return false;

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    *value = entry->value;
    return true;
}

/**
 * capacity で指定された個数のエントリを持つ配列を用意し，ハッシュ表に紐づける．
 *
 * @note capacity が変わると，エントリが格納されるべきインデックスも変化してしまうため，
 *       既存のエントリは，その新しく作成した配列に再記入する．
 *
 * @note 墓標エントリはコピーしても意味がないので，コピーしない．
 */
static void adjustCapacity(Table* table, int capacity) {
    Entry* entries = ALLOCATE(Entry, capacity);

    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    table->count = 0; // NOTE: 墓標エントリがカットされることで，エントリ数も変化する可能性があるため再計算．
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i]; // 古いエントリ配列
        if (entry->key == NULL) continue;

        Entry* dest = findEntry(entries, capacity, entry->key); // 古いエントリの，新しいエントリ配列における格納場所
        dest->key = entry->key;
        dest->value = entry->value;

        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity); // 古いエントリ配列のメモリを解放
    table->entries = entries;
    table->capacity = capacity;
}

bool tableSet(Table* table, ObjString* key, Value value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;

    // NOTE: 墓標エントリの場合は，既にカウント済みなのでインクリメントしない．
    if (isNewKey && IS_NIL(entry->value)) table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

bool tableDelete(Table* table, ObjString* key) {
    if (table->count == 0) return false;

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    /**
     * エントリに墓標（tombstone）を立てる．
     *
     * NOTE: エントリをクリアするのではなく，墓標を立てる理由
     *  単純にエントリをクリアしてしまうと，線形探針が機能しなくなってしまうため，
     *  特別な標準エントリ（sentinel entry）で置き換える．
     *  また，findEntry や adjustCapacity で考慮することが増えるものの，
     *  全体としては，探針チェーンを再構成するよりも高速になる（効率が良い）．
     *
     * NOTE: 墓標エントリは記入済みエントリとして扱うので，table->count はそのままにしておく．
     */
    entry->key = NULL;
    entry->value = BOOL_VAL(true);

    return true;
}

void tableAddAll(Table* from, Table* to) {
    for (int i = 0; i < from->capacity; i++) {
        Entry* entry = &from->entries[i];
        if (entry->key != NULL) {
            tableSet(to, entry->key, entry->value);
        }
    }
}

ObjString* tableFindString(
    Table* table,
    const char* chars,
    int length,
    uint32_t hash
) {
    if (table->count == 0) return NULL;

    uint32_t index = hash & (table->capacity - 1); // note: このハッシュ表のサイズは必ず2のN乗になるので，hash % table->capacity と結果は同じだが，高速化のためにビット演算で実装している．

    for (;;) {
        Entry* entry = &table->entries[index];

        if (entry->key == NULL) {
            // （墓標エントリではない，）空エントリの場合
            if (IS_NIL(entry->value)) return NULL;
        } else if (
            /**
             * （素早い比較）まず，長さとハッシュの比較を行う．
             * （厳密な比較）ハッシュが衝突している可能性もあるので，最後に1文字ずつ比較する．
             */
            entry->key->length == length
            && entry->key->hash == hash
            && memcmp(entry->key->chars, chars, length) == 0
        ) {
            return entry->key;
        }

        // 線形探針
        index = (index + 1) & (table->capacity - 1); // note: このハッシュ表のサイズは必ず2のN乗になるので，(index + 1) % table->capacity と結果は同じだが，高速化のためにビット演算で実装している．
    }
}

void tableRemoveWhite(Table* table) {
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (entry->key != NULL && !entry->key->obj.isMarked) {
            tableDelete(table, entry->key);
        }
    }
}

void markTable(Table* table) {
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        markObject((Obj*)entry->key);
        markValue(entry->value);
    }
}
