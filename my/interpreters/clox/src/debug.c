#include <stdio.h>

#include "debug.h"
#include "object.h"
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

// NOTE: static -> private 関数と同義
// NOTE: const -> 変数を関数内で書き換えられなくする（定数化）
// WARNING: 関数呼び出しより前に関数宣言を配置する必要がある（もしくはプロトタイプ宣言を行う）
static int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

/**
 * チャンクに値が現れない命令をデバッグする（バイトコードをそのまま出力する）．
 *
 * e.g. コンパイラで完結するローカル変数名など
 */
static int byteInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);

    // NOTE: opcode (1 byte) + operand (1 byte) = 2 byte
    return offset + 2;
}

/**
 * @param sign 1: 順方向にジャンプ（スキップ），-1: 逆方向にジャンプ（巻き戻し）
 */
static int jumpInstruction(
    const char* name,
    int sign,
    Chunk* chunk,
    int offset
) {
    // jump: 16 bit のジャンプオフセット
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2]; // a |= b は a = a | b と同義．（a と b をORビット演算した結果を a に格納する．）

    /**
     * offset + 3   現在の命令の次の命令位置
     * sign * jump  ジャンプする距離（sign が 1 or -1 かで順／逆方向が決まる．）
     */
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);

    // NOTE: opcode (1 byte) + operand (2 byte) = 3 byte
    return offset + 3;
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
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_GET_LOCAL:
            return byteInstruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:
            return byteInstruction("OP_SET_LOCAL", chunk, offset);
        case OP_GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_GET_UPVALUE:
            return byteInstruction("OP_GET_UPVALUE", chunk, offset);
        case OP_SET_UPVALUE:
            return byteInstruction("OP_SET_UPVALUE", chunk, offset);

        /**
         * NOTE: 算術演算子（+ など）はスタックからオペランドを取得するが，
         *       算術演算のバイトコード命令（OP_ADD など）自体はオペランドを含まない．
         *       命令が実行されたら，その時にスタックから値をポップすればいいので，オペランドとして保持する必要がないのがポイント．
         */
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBSTRACT:
            return simpleInstruction("OP_SUBSTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);

        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_PRINT:
            return simpleInstruction("OP_PRINT", offset);
        case OP_JUMP:
            return jumpInstruction("OP_JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_LOOP:
            return jumpInstruction("OP_LOOP", -1, chunk, offset);
        case OP_CALL:
            return byteInstruction("OP_CALL", chunk, offset);
        case OP_CLOSURE: {
            offset++;
            uint8_t constant = chunk->code[offset++];
            printf("%-16s %4d ", "OP_CLOSURE", constant);
            printValue(chunk->constants.values[constant]);
            printf("\n");

            ObjFunction* function = AS_FUNCTION(chunk->constants.values[constant]);
            for (int j = 0; j < function->upvalueCount; j++) {
                int isLocal = chunk->code[offset++];
                int index = chunk->code[offset++];
                printf(
                    "%04d      |                     %s %d\n",
                    offset - 2, // オペランド（isLocal, index）で2バイトあるので，その分戻る．
                    isLocal ? "local" : "upvalue",
                    index
                );
            }

            return offset;
        }
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
