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

ObjString* makeString(bool ownsChars, char* chars, int length) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->ownsChars = ownsChars;
    string->length = length;
    string->chars = chars;
    return string;
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            printf("%.*s", AS_STRING(value)->length, AS_CSTRING(value));
            break;
    }
}
