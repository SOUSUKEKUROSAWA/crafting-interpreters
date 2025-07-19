#include <stdio.h>

#include "common.h"
#include "vm.h"

// NOTE: 関数全てにVMへのポインタを渡すのは厄介なので，グローバルオブジェクトとして定義
VM vm;

void initVM() {}

void freeVM() {}

// NOTE: VM のコアとなる関数
static InterpretResult run() {

// 現在 ip が指している命令を読みだしてから，ip を１つ次に進める．
#define READ_BYTE() (*vm.ip++)

/**
 * 現在 ip が指している命令を読みだし，
 * その値をインデックスとして持つチャンクの定数プールから，
 * 対応する値を取り出す．
 * （ip を１つ次に進め）
 */
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

    for (;;) {
        uint8_t instruction;

        // 命令（バイトコード）のデコード（ディスパッチ）を行う．
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                printValue(constant);
                printf("\n");
                break;
            }
            case OP_RETURN: {
                return INTERPRET_OK;
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
}

InterpretResult interpret(Chunk* chunk) {
    vm.chunk = chunk;
    vm.ip = vm.chunk->code; // チャンクの最初のバイトコード（命令）へのポインタを渡すことで初期化する．
    return run();
}
