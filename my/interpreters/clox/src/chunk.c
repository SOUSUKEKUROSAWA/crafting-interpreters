#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lineCount = 0;
    chunk->lineCapacity = 0;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(LineStart, chunk->lines, chunk->lineCapacity);
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
    }

    chunk->code[chunk->count] = byte;
    chunk->count++;

    if (
        chunk->lineCount > 0
        && chunk->lines[chunk->lineCount - 1].line == line
    ) {
        // 一つ前のバイトコードと同じ行なら何もしない（情報の圧縮）
        return;
    }

    if (chunk->lineCapacity < chunk->lineCount + 1) {
        int oldCapacity = chunk->lineCapacity;
        chunk->lineCapacity = GROW_CAPACITY(oldCapacity);
        chunk->lines = GROW_ARRAY(
            LineStart,
            chunk->lines,
            oldCapacity,
            chunk->lineCapacity
        );
    }

    LineStart* lineStart = &chunk->lines[chunk->lineCount++]; // 変数に，書き込むべきメモリ領域の参照を渡す
    lineStart->offset = chunk->count - 1;
    lineStart->line = line;
}

int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);

    // 追加した定数を後で参照できるように，定数を置いた位置のインデックスを返す．
    // NOTE: writeValueArray 内で count がインクリメントされているので，-1 を付ける必要がある
    return chunk->constants.count -1;
}

int getLine(Chunk* chunk, int instruction) {
    int start = 0;
    int end = chunk->lineCount - 1;

    // バイナリサーチ（二分探索）
    // WARNING: 探索対象（行番号）が昇順に並んでいることが前提条件．
    for (;;) {
        int mid = (start + end) / 2;
        LineStart* line = &chunk->lines[mid];
        if (instruction < line->offset) {
            end = mid - 1;
        } else if (
            mid == chunk->lineCount - 1 // チャンクの終端に達した場合
            || instruction < chunk->lines[mid + 1].offset // 次の行の開始点（＝現在の行の終端）よりも手前にいる場合
        ) {
            return line->line;
        } else {
            start = mid + 1;
        }
    }
}
