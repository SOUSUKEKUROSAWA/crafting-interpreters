#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

/**
 * ハッシュ表の要素
 * キーと値のペア
 */
typedef struct {
    ObjString* key; // @note キーは常に文字列なので，効率化のため Value（タグ付き共用体）でラップしない．
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

/**
 * ハッシュ表から所与のキーに一致するエントリの値を取得する．
 *
 * @param value 出力パラメータ．キーに一致するエントリの値が呼び出し元で取得できるように，このパラメータにコピーされる．
 * @return true: キーを持つエントリが存在した, false: 存在しなかった
 */
bool tableGet(Table* table, ObjString* key, Value* value);

/**
 * ハッシュ表にエントリを追加／上書きする．
 *
 * @return true: 新規エントリーの追加, false: 既存のエントリーの上書き
 */
bool tableSet(Table* table, ObjString* key, Value value);

bool tableDelete(Table* table, ObjString* key);

/**
 * あるハッシュ表（from）の全てのエントリを別のハッシュ表（to）へ追加／上書きする．
 */
void tableAddAll(Table* from, Table* to);

/**
 * 所与の文字列がインターン化済みかどうかを確認する．
 *
 * @param table インターン化済みの文字列の一覧（ハッシュ表）
 * @param chars 探しているキーを表現する生の文字配列
 * @return インターン化済みの場合: そのキーへのポインタ，未インターン化の場合: NULL
 * @note 関数呼び出し時点では，まだ ObjString を作成していない．
 */
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);

/**
 * 所与のハッシュ表の内，キーの文字列オブジェクトが白オブジェクト
 * （マークされていないオブジェクト）であれば，表から削除して，
 * 参照切れのポインタが残らないようにする．
 *
 * ref. 三色抽象化
 */
void tableRemoveWhite(Table* table);

void markTable(Table* table);

#endif
