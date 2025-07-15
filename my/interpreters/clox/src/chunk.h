#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

/**
 * NOTE: バイトコード = 個々の命令が 1 バイトの演算コード（operation code, opcode）
 * NOTE: operand = opcode の直後に置かれ，opcode ごとに異なる意味を持つパラメータ
 */
typedef enum {
    OP_CONSTANT, // 特定の数を生成する．operand は生成する変数を表す．
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
    uint8_t* code; // 8 bit（1 byte）符号なし整数型へのポインタ
    ValueArray constants; // 定数プール（値の配列）
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte);
int addConstant(Chunk* chunk, Value value);

#endif
