#include "common.h"
#include "chunk.h"
#include "debug.h"

int main(int argc, const char* argv[]) {
    Chunk chunk;
    initChunk(&chunk);

    // 定数プールを 1 バイト（8 bit = 256）以上のサイズにする
    for (int i = 0; i < 500; i++) {
        addConstant(&chunk, (double)i);
    }

    // その状態で追加の定数が上手く扱えればOK
    writeConstant(&chunk, 1.2, 123);

    writeChunk(&chunk, OP_RETURN, 123);

    disassembleChunk(&chunk, "test chunk");

    freeChunk(&chunk);
    return 0;
}
