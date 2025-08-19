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

ObjString* makeString(int length) {
    ObjString* string = (ObjString*)allocateObject(
        sizeof(ObjString) + length + 1, OBJ_STRING
    );
    string->length = length;
    return string;
}

/**
 * 渡された文字列をヒープ上にコピーして ObjString に割り当てる（所有しない）．
 *
 * @note 渡される文字列がソース文字列の一部のような場合に使う．
 */
ObjString* copyString(const char* chars, int length) {
    ObjString* string = makeString(length);

    // 配列に字句をコピー
    memcpy(string->chars, chars, length);

    // NOTE: ソース文字列の一部の参照の可能性があるので，ターミネータを明示的に追加
    string->chars[length] = '\0';

    return string;
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING: printf("%s", AS_CSTRING(value)); break;
    }
}
