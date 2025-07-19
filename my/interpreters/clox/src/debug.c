#include <stdio.h>

#include "debug.h"
#include "value.h"

void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);
    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1]; // 定数プールの中でのその定数の位置（インデックス）
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");

    // NOTE: opcode (1 byte) + operand (1 byte) = 2 byte
    return offset + 2;
}

static int longConstantInstruction(
    const char* name,
    Chunk* chunk,
    int offset
) {
    uint32_t constant = chunk->code[offset + 1]
        | (chunk->code[offset + 2] << 8)
        | (chunk->code[offset + 3] << 16);
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");

    // NOTE: opcode (1 byte) + operand (3 byte) = 4 byte
    return offset + 4;
}

// NOTE: static -> private 関数と同義
// NOTE: const -> 変数を関数内で書き換えられなくする（定数化）
// WARNING: 関数呼び出しより前に関数宣言を配置する必要がある（もしくはプロトタイプ宣言を行う）
static int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

int disassembleInstruction(Chunk* chunk, int offset) {
    printf("%04d ", offset);

    if (
        offset > 0
        && chunk->lines[offset] == chunk->lines[offset - 1]
    ) {
        // 一つ前のバイトコードと同じ行の場合は表示の重複を避ける．
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_CONSTANT_LONG:
            return longConstantInstruction("OP_CONSTANT_LONG", chunk, offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
