#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

/**
 * @note Obj* ではなく，指定した type としてポインタを返すためにマクロでラップしている．
 */
#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;
    object->isMarked = false;
    object->next = vm.objects; // 末尾ではなく，先頭に順に繋いでいく．
    vm.objects = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

    return object;
}

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method) {
    ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

ObjClass* newClass(ObjString* name) {
    // @note class が予約語である C++ などでもコンパイルしやすくするために，あえて klass という変数名にしている．
    ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name;
    initTable(&klass->methods);
    return klass;
}

ObjClosure* newClosure(ObjFunction* function) {
    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

ObjFunction* newFunction() {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

ObjInstance* newInstance(ObjClass* klass) {
    ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    initTable(&instance->fields);
    return instance;
}

ObjNative* newNative(NativeFn function) {
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
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

    push(OBJ_VAL(string)); // GC が勝手にメモリを開放しないように一旦VMのスタックにプッシュする．
    tableSet(&vm.strings, string, NIL_VAL); // NOTE: 値はどうでもいいので nil を使う．
    pop();

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

ObjUpvalue* newUpvalue(Value* slot) {
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed = NIL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;
    return upvalue;
}

static void printFunction(ObjFunction* function) {
    if (function->name == NULL) {
        // トップレベルコードの場合（関数名の有無で判別している）
        printf("<script>");
        return;
    }

    printf("<fn %s>", function->name->chars);
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD: printFunction(AS_BOUND_METHOD(value)->method->function); break;
        case OBJ_CLASS: printf("%s", AS_CLASS(value)->name->chars); break;
        case OBJ_CLOSURE: printFunction(AS_CLOSURE(value)->function); break;
        case OBJ_FUNCTION: printFunction(AS_FUNCTION(value)); break;
        case OBJ_INSTANCE: printf("%s instance", AS_INSTANCE(value)->klass->name->chars); break;
        case OBJ_NATIVE: printf("<native fn>"); break;
        case OBJ_STRING: printf("%s", AS_CSTRING(value)); break;
        case OBJ_UPVALUE: printf("upvalue"); break;
    }
}
