#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
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

static ObjString* allocateString(char* chars, int length) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    return string;
}

/**
 * 渡された文字列をそのまま ObjString に割り当てる（所有する）．
 *
 * @note すでにヒープに割り当て済みの文字列に対して使う．
 */
ObjString* takeString(char* chars, int length) {
    return allocateString(chars, length);
}

/**
 * 渡された文字列をヒープ上にコピーして ObjString に割り当てる（所有しない）．
 *
 * @note 渡される文字列がソース文字列の一部のような場合に使う．
 */
ObjString* copyString(const char* chars, int length) {
    // ヒープ上に新しい配列を割り当てる．ターミネータも含むので length + 1
    char* heapChars = ALLOCATE(char, length + 1);

    // 配列に字句をコピー
    memcpy(heapChars, chars, length);

    // NOTE: ソース文字列の一部の参照の可能性があるので，ターミネータを明示的に追加
    heapChars[length] = '\0';

    return allocateString(heapChars, length);
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING: printf("%s", AS_CSTRING(value)); break;
    }
}
