#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

// NOTE: Obj* ではなく，指定した type としてポインタを返すためにマクロでラップしている．
#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;
    // 末尾ではなく，先頭に順に繋いでいく．
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

ObjFunction* newFunction() {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

static ObjString* allocateString(
    char* chars,
    int length,
    uint32_t hash
) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    tableSet(&vm.strings, string, NIL_VAL); // NOTE: 値はどうでもいいので nil を使う．
    return string;
}

/**
 * @note ハッシュ関数: FNV-1a
 */
static uint32_t hashString(const char* key, int length) {
    uint32_t hash = 2166136261u; // 初期値（数学的な特性を慎重に検討して選択された定数）
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i]; // 現在のハッシュ値（hash）と文字列のi番目の文字（key[i]）の XOR を計算する．
        hash *= 16777619; // FNV prime（分散を良くするための大きな定数）を掛ける．
    }
    return hash;
}

/**
 * 渡された文字列をそのまま ObjString に割り当てる（所有する）．
 *
 * @note すでにヒープに割り当て済みの文字列に対して使う．
 */
ObjString* takeString(char* chars, int length) {
    uint32_t hash = hashString(chars, length);

    // 文字列がすでにインターン化されていれば，所有権を解放したうえで，そのポインタを返す．
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return allocateString(chars, length, hash);
}

/**
 * 渡された文字列をヒープ上にコピーして ObjString に割り当てる（所有しない）．
 *
 * @note 渡される文字列がソース文字列の一部のような場合に使う．
 */
ObjString* copyString(const char* chars, int length) {
    uint32_t hash = hashString(chars, length);

    // 文字列がすでにインターン化されていれば，そのポインタを返す．
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) return interned;

    // ヒープ上に新しい配列を割り当てる．ターミネータも含むので length + 1
    char* heapChars = ALLOCATE(char, length + 1);

    // 配列に字句をコピー
    memcpy(heapChars, chars, length);

    // NOTE: ソース文字列の一部の参照の可能性があるので，ターミネータを明示的に追加
    heapChars[length] = '\0';

    return allocateString(heapChars, length, hash);
}

static void printFunction(ObjFunction* function) {
    printf("<fn %s>", function->name->chars);
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_FUNCTION: printFunction(AS_FUNCTION(value)); break;
        case OBJ_STRING: printf("%s", AS_CSTRING(value)); break;
    }
}
