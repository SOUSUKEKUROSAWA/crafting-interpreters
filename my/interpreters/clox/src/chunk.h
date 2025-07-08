#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"

// NOTE: バイトコードは，個々の命令が 1 バイトの演算コード（operation code, opcode）
typedef enum {
    OP_RETURN,
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
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte);

#endif
