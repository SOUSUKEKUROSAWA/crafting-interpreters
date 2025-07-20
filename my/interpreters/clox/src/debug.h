#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.h"

// チャンク全体に含まれる全ての命令を逆アセンブルする
void disassembleChunk(Chunk* chunk, const char* name);

/**
 * 1個の命令を逆アセンブルする
 * NOTE: 逆アセンブル = 高レベルのコードをより低レベル（機械語に近いレベル）の命令に変換すること
 * @param offset チャンクの先頭から現在の命令までの間のバイト数
 * @return 次の命令の位置
 */
int disassembleInstruction(Chunk* chunk, int offset);

#endif
