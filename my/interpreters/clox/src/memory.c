#include <stdlib.h>

#include "memory.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
        collectGarbage();
#endif
    }

    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    // NOTE: pointer には，そのポインタが指すメモリブロックのサイズをメタデータとして持っている．
    void* result = realloc(pointer, newSize);

    if (result == NULL) exit(1);

    return result;
}

void markObject(Obj* object) {
    if (object == NULL) return;

#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif

    object->isMarked = true;
}

void markValue(Value value) {
    // OBJ ではないものは（数値，ブール値，NIL），Value に直接インラインで置かれるので，ヒープ割り当てを必要としない．
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

static void freeObject(Obj* object) {
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)object, object->type);
#endif

    switch (object->type) {
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            // NOTE: ObjUpvalue そのものは所有しないが，上位値へのポインタを含む配列は所有するので，ここで解放する．
            FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
            // NOTE: ObjFunction は所有していないので解放もしない．
            FREE(ObjClosure, object);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            freeChunk(&function->chunk);
            FREE(ObjFunction, object);
            break;
        }
        case OBJ_NATIVE: FREE(ObjNative, object); break;
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }
        case OBJ_UPVALUE: FREE(ObjUpvalue, object); break;
    }
}

/**
 * 直接参照可能なルートオブジェクトの走査
 */
static void markRoots() {
    // スタック上の値（ローカル変数，一時的な値）のマーキング
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        markValue(*slot);
    }

    // グローバル変数上の値のマーキング
    markTable(&vm.globals);
}

void collectGarbage() {
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
#endif

    markRoots();

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
#endif
}

void freeObjects() {
    Obj* object = vm.objects;

    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }
}
