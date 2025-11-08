#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

/**
 * NOTE: バイトコード = 個々の命令が 1 バイトの演算コード（operation code, opcode）
 * NOTE: operand = opcode の直後に置かれ，opcode ごとに異なる意味を持つパラメータ
 */
typedef enum {
    OP_CONSTANT, // 特定の数をスタックにプッシュする．@operand プッシュする値
    OP_NIL, // nil をスタックにプッシュする．
    OP_TRUE, // true をスタックにプッシュする．
    OP_FALSE, // false をスタックにプッシュする．
    OP_POP, // スタックから値をポップする．
    OP_GET_LOCAL, // ローカル変数をスタックトップにプッシュし直して，後の命令で見つけられるようにする．@operand ローカル変数が存在するスタック上のインデックス
    OP_SET_LOCAL, // スタックトップにある値でローカル変数を上書きする．@operand ローカル変数が存在するスタック上のインデックス
    OP_GET_GLOBAL, // グローバル変数を取得し，スタックにプッシュする．@operand 取得する変数名が格納されている定数表のインデックス
    OP_DEFINE_GLOBAL, // グローバル変数を保存する．@operand 保存する変数名が格納されている定数表のインデックス
    OP_SET_GLOBAL, // グローバル変数を上書きする．@operand 上書きする変数名が格納されている定数表のインデックス
    OP_GET_UPVALUE, // @operand 取得する上位値が格納されているインデックス
    OP_SET_UPVALUE, // @operand 上書きする上位値が格納されているインデックス
    OP_EQUAL, // == （NOTE: OP_NOT と組み合わせることで != を表現可能）
    OP_GREATER, // > （NOTE: OP_NOT と組み合わせることで <= を表現可能）
    OP_LESS, // < （NOTE: OP_NOT と組み合わせることで >= を表現可能）
    OP_ADD, // 加算
    OP_SUBSTRACT, // 減算
    OP_MULTIPLY, // 乗算
    OP_DIVIDE, // 除算
    OP_NOT, // (!) 否定
    OP_NEGATE, // (-) 正負逆転演算．
    OP_PRINT, // スタックから値をポップして出力する．
    OP_JUMP, // 特定の数だけ命令をスキップする．@operand スキップする命令数（※ 2 バイト）
    OP_JUMP_IF_FALSE, // スタックから取得した値が Falsey であれば，特定の数だけ命令をスキップする．@operand スキップする命令数（※ 2 バイト）
    OP_LOOP, // 特定の数だけ命令を巻き戻す．@operand 巻き戻す命令数（※ 2 バイト）
    OP_CALL, // 呼び出された関数のためのコールフレームをフレームスタックに追加する．@operand 引数の個数
    OP_CLOSURE, // クロージャをスタックにプッシュする．@operand 定数表内の関数が置かれた位置（インデックス）＋クロージャがキャプチャする個々の上位値の情報（2バイト，個数は可変）
    OP_CLOSE_UPVALUE, // スタックトップにあるローカル変数をヒープに移す．
    OP_RETURN, // 現在の関数からリターンする．
    OP_CLASS, // 指定されたクラス名のクラスオブジェクトを作成する．@operand クラス名の定数表のインデックス．
} OpCode;

/**
 * NOTE: chunk = バイトコードのシーケンス
 * NOTE: 動的配列にバイトコードを収めることで，
 * キャッシュ効率の高い密なストレージに命令を収められて，
 * 高速に実行できる．
 */
typedef struct {
    int count; // 要素数（利用済みの容量）
    int capacity; // 総容量

    // NOTE: 実行時にサイズが変化するので，動的配列として実装し，ポインタを保持する．
    uint8_t* code; // バイトコード（動的配列）の先頭へのポインタ
    int* lines; // 各バイトコードのソースコード上での行情報（動的配列）の先頭へのポインタ

    ValueArray constants; // 定数プール（値の配列）
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);

#endif
