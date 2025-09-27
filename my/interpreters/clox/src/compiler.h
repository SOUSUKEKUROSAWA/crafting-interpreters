#ifndef clox_compiler_h
#define clox_compiler_h

#include "object.h"
#include "vm.h"

/**
 * @note コンパイラ自身が（ObjFunction にラップされた）チャンクを作成して，返す．
 */
ObjFunction* compile(const char* source);

#endif
