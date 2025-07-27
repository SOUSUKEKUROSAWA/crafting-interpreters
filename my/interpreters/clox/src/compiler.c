#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

void compile(const char* source) {
    initScanner(source);

    int line = -1;
    for (;;) {
        // トークンの読み込み
        Token token = scanToken();

        // 行番号の表示
        if (token.line != line) {
            printf("%4d ", token.line);
            line = token.line;
        } else {
            printf("   | ");
        }

        /**
         * トークンの表示
         *
         * %.*s について
         *  token.length で表示する文字数を指定し，
         *  token.start で開始位置を指定している．
         */
        printf("%2d '%.*s'\n", token.type, token.length, token.start);

        if (token.type == TOKEN_EOF) break;
    }
}
