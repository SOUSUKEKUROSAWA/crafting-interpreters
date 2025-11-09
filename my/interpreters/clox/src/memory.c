#include <stdlib.h>

#include "compiler.h"
#include "memory.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2 // 次のGCの閾値を現在の使用しているヒープメモリサイズの何倍に設定するか．

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    vm.bytesAllocated += newSize - oldSize;

    if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
        collectGarbage();
#endif

        // ref. 自己調整ヒープ
        if (vm.bytesAllocated > vm.nextGC) {
            collectGarbage();
        }
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
    if (object->isMarked) return; // オブジェクト参照が閉路になっている場合の無限ループを防ぐ．

#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif

    object->isMarked = true;

    // ref. 三色抽象化
    if (vm.grayCapacity < vm.grayCount + 1) {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        /**
         * WARNING: grayStack の拡大が原因でGCを再帰的に起動させてしまわないように，
         *          reallocate ではなく，システムの realloc を直接呼び出す．
         */
        vm.grayStack = (Obj**)realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity);
    }

    vm.grayStack[vm.grayCount++] = object;

    if (vm.grayStack == NULL) exit(1); // アロケーションの失敗．
}

void markValue(Value value) {
    // OBJ ではないものは（数値，ブール値，NIL），Value に直接インラインで置かれるので，ヒープ割り当てを必要としない．
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

static void markArray(ValueArray* array) {
    for (int i = 0; i < array->count; i++) {
        markValue(array->values[i]);
    }
}

/**
 * 所与のオブジェクトの参照を辿って暗黒化（blacken）する．
 *
 * @note 暗黒化：白オブジェクトをグレーに，グレーオブジェクトを黒に変える．ref. 三色抽象化
 * @note 黒オブジェクト：isMarked が true の状態で，グレースタックから外れたオブジェクトのこと．
 */
static void blackenObject(Obj* object) {
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif

    switch (object->type) {
        // note: 変数宣言を含む文は，スコープの関係で {} で囲む必要がある．
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            markObject((Obj*)closure->function);
            for (int i = 0; i < closure->upvalueCount; i++) {
                markObject((Obj*)closure->upvalues[i]);
            }
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            markObject((Obj*)function->name);
            markArray(&function->chunk.constants); // 関数内で使用する他のオブジェクトへの参照群．
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            markObject((Obj*)instance->klass);
            markTable(&instance->fields);
            break;
        }
        case OBJ_UPVALUE:
            markValue(((ObjUpvalue*)object)->closed);
            break;

        // オブジェクト参照を含まないので，さらに辿るべきものがないタイプ
        case OBJ_NATIVE:
        case OBJ_STRING:
            break;
    }
}

static void freeObject(Obj* object) {
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)object, object->type);
#endif

    switch (object->type) {
        case OBJ_CLASS: {
            FREE(ObjClass, object);
            break;
        }
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
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            freeTable(&instance->fields); // フィールド内のエントリそのものの解放はGCに任せる．
            FREE(ObjInstance, object);
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
 * 直接参照可能なルートオブジェクトを走査し，マーキングとグレースタックへの追加を行う．
 *
 * ref. 三色抽象化
 */
static void markRoots() {
    // スタック上の値（ローカル変数，一時的な値）
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        markValue(*slot);
    }

    // コールフレームのスタック
    for (int i = 0; i < vm.frameCount; i++) {
        markObject((Obj*)vm.frames[i].closure);
    }

    // オープン上位値のリスト
    for (
        ObjUpvalue* upvalue = vm.openUpvalues;
        upvalue != NULL;
        upvalue = upvalue->next
    ) {
        markObject((Obj*)upvalue);
    }

    // グローバル変数のハッシュ表
    markTable(&vm.globals);

    // コンパイラが直接アクセスする値
    markCompilerRoots();
}

/**
 * グレースタックの中身がなくなるまで，参照を辿って，マーキングとグレースタックへの削除・追加を行う．
 *
 * @note 削除だけでなく，グレースタックへの追加も行われる．最終的にグレースタックが無くなるまで，ループが続く．
 *       ref. 三色抽象化
 */
static void traceReferences() {
    while (vm.grayCount > 0) {
        Obj* object = vm.grayStack[--vm.grayCount]; // note: 後置デクリメント：先にデクリメントしてから，その新しい値をインデックスに使う．
        blackenObject(object);
    }
}

/**
 * 白オブジェクトのメモリ割り当てを解放し，再利用可能な状態にする．
 * ref. 三色抽象化
 */
static void sweep() {
    Obj* previous = NULL;
    Obj* object = vm.objects;
    while (object != NULL) {
        if (object->isMarked) {
            object->isMarked = false; // 次回のGCのために，マークをクリアする．
            previous = object;
            object = object->next;
        } else {
            Obj* unreached = object;

            // オブジェクトチェーンの繋ぎ直し
            object = object->next;
            if (previous != NULL) {
                previous->next = object;
            } else {
                // 先頭のオブジェクトを解放する場合
                vm.objects = object;
            }

            freeObject(unreached);
        }
    }
}

void collectGarbage() {
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = vm.bytesAllocated;
#endif

    markRoots();
    traceReferences();
    tableRemoveWhite(&vm.strings);
    sweep();

    // GCがメモリを解放する時にも reallocate() は呼ばれるので，
    // この時点で vm.bytesAllocated は解放後のバイト数に一致している．
    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    printf("   collocted %zu bytes (from %zu to %zu) next at %zu\n",
        before - vm.bytesAllocated, before, vm.bytesAllocated, vm.nextGC
    );
#endif
}

void freeObjects() {
    Obj* object = vm.objects;

    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }

    free(vm.grayStack);
}
