#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
    const char* start; // 今スキャンしている字句の先頭へのポインタ
    const char* current; // 今見ている文字へのポインタ
    int line; // エラー報告のために現在行を追跡
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}
