#ifndef clox_compiler_h
#define clox_compiler_h

#include "object.h"
#include "vm.h"

/**
 * @return 正常時は ObjFunction でラップしたチャンクを，エラー時には NULL を返す．
 * @note コンパイラ自身が（ObjFunction にラップされた）チャンクを作成して，返す．
 */
ObjFunction* compile(const char* source);

void markCompilerRoots();

#endif
