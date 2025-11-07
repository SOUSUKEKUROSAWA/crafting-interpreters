#include <stdlib.h>

#include "chunk.h"
#include "memory.h"
#include "vm.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
    // 容量が足りない場合は，より大きな配列に値をコピーし，参照を移す
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(
            uint8_t,
            chunk->code,
            oldCapacity,
            chunk->capacity
        );
        chunk->lines = GROW_ARRAY(
            int,
            chunk->lines,
            oldCapacity,
            chunk->capacity
        );
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line; // NOTE: 行情報はバイトコードと同じインデックスでアクセスできる

    chunk->count++;
}

int addConstant(Chunk* chunk, Value value) {
    // 引数 value はCのスタックにあり，GC からは隠されているので，
    // GC が勝手にメモリを開放しないように一旦VMのスタックにプッシュする．
    push(value);
    writeValueArray(&chunk->constants, value);
    pop();

    // 追加した定数を後で参照できるように，定数を置いた位置のインデックスを返す．
    // NOTE: writeValueArray 内で count がインクリメントされているので，-1 を付ける必要がある
    return chunk->constants.count -1;
}
