#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

static void repl() {
    char line[1024];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            // 読み込む文字列がなくなる or 読み込みに失敗した場合
            printf("\n");
            break;
        }

        interpret(line);
    }
}

static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    /**
     * ファイル全体を読み込めるほど大きな文字列を割り当てたいが，
     * ファイルの大きさは読み終わるまでわからないという問題を解決するために，
     * はじめにファイルの末尾までシークし（fseek），
     * ファイルの先頭から何バイト離れたかを確認している（ftell）．
     * そこまで終ったら，ファイルの先頭まで巻き戻す（rewind）．
     */
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1); // 末尾には null バイトがあるので，+1
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

static void runFile(const char* path) {
    char* source = readFile(path);
    InterpretResult result = interpret(source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR) exit (65);
    if (result == INTERPRET_RUNTIME_ERROR) exit (70);
}

int main(int argc, const char* argv[]) {
    initVM();

    // WARNING: argv の第一引数は常に実行ファイル名になる．
    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

    freeVM();
    return 0;
}
