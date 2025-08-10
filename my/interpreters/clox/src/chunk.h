#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

/**
 * NOTE: バイトコード = 個々の命令が 1 バイトの演算コード（operation code, opcode）
 * NOTE: operand = opcode の直後に置かれ，opcode ごとに異なる意味を持つパラメータ
 */
typedef enum {
    OP_CONSTANT, // 特定の数をスタックにプッシュする．operand はプッシュする値を表す．
    OP_NIL, // nil をスタックにプッシュする．
    OP_TRUE, // true をスタックにプッシュする．
    OP_FALSE, // false をスタックにプッシュする．
    OP_EQUAL, // == （NOTE: OP_NOT と組み合わせることで != を表現可能）
    OP_GREATER, // > （NOTE: OP_NOT と組み合わせることで <= を表現可能）
    OP_LESS, // < （NOTE: OP_NOT と組み合わせることで >= を表現可能）
    OP_ADD, // 加算
    OP_SUBSTRACT, // 減算
    OP_MULTIPLY, // 乗算
    OP_DIVIDE, // 除算
    OP_NOT, // (!) 否定
    OP_NEGATE, // (-) 正負逆転演算．
    OP_RETURN, // 現在の関数からリターンする．
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
