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
    return object;
}

static ObjString* allocateString(char* chars, int length) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    return string;
}

ObjString* copyString(const char* chars, int length) {
    // ヒープ上に新しい配列を割り当てる．ターミネータも含むので length + 1
    char* heapChars = ALLOCATE(char, length + 1);

    // 配列に字句をコピー
    memcpy(heapChars, chars, length);

    // NOTE: ソース文字列の一部の参照の可能性があるので，ターミネータを明示的に追加
    heapChars[length] = '\0';

    return allocateString(heapChars, length);
}
