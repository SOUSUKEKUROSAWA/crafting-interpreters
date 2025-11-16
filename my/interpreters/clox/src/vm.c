#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

// NOTE: 関数全てにVMへのポインタを渡すのは厄介なので，グローバルオブジェクトとして定義
VM vm;

static Value clockNative(int argCount, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static void resetStack() {
    vm.stackTop = vm.stack; // NOTE: vm.stack はスタック配列の先頭アドレスを表す．
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
}

/**
 * 可変長引数のエラーログ関数
 *
 * @note printf と同じ書式でメッセージを受け取る
 */
static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format); // 可変長引数の取り出しを開始
    vfprintf(stderr, format, args);
    va_end(args); // 可変長引数の取り出しを終了（クリーンアップ）
    fputs("\n", stderr);

    // NOTE: Stack Trace
    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;

        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    resetStack();
}

static void defineNative(const char* name, NativeFn function) {
    // GC が勝手にメモリを開放しないように一旦VMのスタックにプッシュする．
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;
    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;

    initTable(&vm.globals);
    initTable(&vm.strings);

    vm.initString = NULL;
    vm.initString = copyString("init", 4);

    defineNative("clock", clockNative);
}

void freeVM() {
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    vm.initString = NULL;
    freeObjects();
}

void push(Value value) {
    *vm.stackTop = value; // NOTE: こちらはポインタが示す値の変更．
    vm.stackTop++; // NOTE: こちらはポインタ自体の変更．
}

Value pop() {
    vm.stackTop--; // 実際に削除はしない．
    return *vm.stackTop;
}

/**
 * distance = 0 の場合，スタックの一番上から Value を取得する．
 * distance = 1 の場合は，その一つ下．
 *
 * @note 消費はしない．
 */
static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

static bool call(ObjClosure* closure, int argCount) {
    if (argCount != closure->function->arity) {
        runtimeError("Expected %d arguments but got %d.", closure->function->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1; // スタックスロットの 0 番目は予約済み（関数オブジェクト自身が配置される）ので -1 が必要．
    return true;
}

/**
 * @return
 *   true: 関数の呼び出し成功，呼び出された関数のためのコールフレームが追加された．
 *  false: 関数の呼び出し失敗（Callable でない）．
 */
static bool callValue(Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
                /**
                 * this の解決のために，メソッドのクロージャをレシーバ（メソッドに紐づくインスタンス）で置換する．
                 * @see compiler.c initCompiler()
                 */
                vm.stackTop[-argCount - 1] = bound->receiver;
                return call(bound->method, argCount);
            }
            case OBJ_CLASS: {
                ObjClass* klass = AS_CLASS(callee);
                vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));

                Value initializer;
                if (tableGet(&klass->methods, vm.initString, &initializer)) {
                    return call(AS_CLOSURE(initializer), argCount);
                } else if (argCount != 0) {
                    runtimeError("Expected 0 arguments but got %d.", argCount);
                    return false;
                }

                return true;
            }
            case OBJ_CLOSURE:
                return call(AS_CLOSURE(callee), argCount);
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                // Cの関数として呼び出して（Cに制御を委ねて），結果を受け取る．
                Value result = native(argCount, vm.stackTop - argCount);
                vm.stackTop -= argCount + 1;
                push(result);
                return true;
            }
            default:
                break;
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

/**
 * 所与の name に一致するメソッドを呼び出す．
 */
static bool invokeFromClass(ObjClass* klass, ObjString* name, int argCount) {
    Value method;
    if (!tableGet(&klass->methods, name, &method)) {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }
    return call(AS_CLOSURE(method), argCount);
}

/**
 * @return true: 呼び出し成功．false: 呼び出し失敗．
 */
static bool invoke(ObjString* name, int argCount) {
    Value receiver = peek(argCount);

    if (!IS_INSTANCE(receiver)) {
        runtimeError("Only instances have methods.");
        return false;
    }

    ObjInstance* instance = AS_INSTANCE(receiver);

    Value value;
    if (tableGet(&instance->fields, name, &value)) {
        /**
         * 関数が格納されているフィールドを呼び出す場合
         *
         * e.g.
         *  class Hoge {
         *      fun f() { fuga; }
         *      this.field = f;
         *  }
         *  var hoge = Hoge();
         *  hoge.field();
         */
        vm.stackTop[-argCount - 1] = value;
        return callValue(value, argCount);
    }

    return invokeFromClass(instance->klass, name, argCount);
}

/**
 * @return true: 所与のメソッドが存在し，スタックトップのレシーバ（メソッドが所属するインスタンス）を束縛メソッドで置換する．
 *         false: 所与のメソッドが存在しなかった．
 */
static bool bindMethod(ObjClass* klass, ObjString* name) {
    Value method;
    if (!tableGet(&klass->methods, name, &method)) {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }

    ObjBoundMethod* bound = newBoundMethod(peek(0), AS_CLOSURE(method));
    pop(); // receiver (=Instance)
    push(OBJ_VAL(bound));
    return true;
}

static ObjUpvalue* captureUpvalue(Value* local) {
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = vm.openUpvalues;
    // まずは既存の上位値リストを探索
    while (upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }
    // 既にキャプチャ済みであればそのポインタを返す
    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue* createdUpvalue = newUpvalue(local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == NULL) {
        vm.openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

/**
 * 所与のスタックスロットにあるローカル変数（open upvalue）か，
 * スタックでそれより上にあるスロットを指すローカル変数（open upvalue）を探して，
 * それら全てをヒープに移す（クローズする）．
 *
 * @param last クローズ対象の境界となるスタックスロットのアドレス．
 */
static void closeUpvalues(Value* last) {
    /**
     * open upvalue のリストをトップからボトムに向けて辿って，
     * 範囲内の上位値をすべてクローズする．
     */
    while (
        vm.openUpvalues != NULL
        && vm.openUpvalues->location >= last
    ) {
        ObjUpvalue* upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location; // 変数の値をヒープ（closed）にコピーする．
        upvalue->location = &upvalue->closed; // 参照先をスタックからヒープ（closed）に切り替える．
        vm.openUpvalues = upvalue->next; // open upvalue のリストから削除する（処理済みの upvalue をリストの追跡から外す）．
    }
}

static void defineMethod(ObjString* name) {
    Value method = peek(0); // クロージャ
    ObjClass* klass = AS_CLASS(peek(1));
    tableSet(&klass->methods, name, method);
    pop(); // クロージャをクリアする．
}

/**
 * @return true: 入力が Falsey な値（nil or false）, false: 入力が Falsey ではない値
 */
static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
    /**
     * @warning pop で取得すると，結合した文字列をヒープに割り当てるタイミングで
     *          GCが動作した場合に，元の文字列が失われてしまうリスクがある．
     */
    ObjString* b = AS_STRING(peek(0));
    ObjString* a = AS_STRING(peek(1));

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1); // ターミネータ（\0）のために + 1
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length); // chars の先頭から a->length バイト進んだ位置から，b の内容を b->length バイト分コピーする．
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    // 連結した文字列ヒープに割り当ててから，元の値をスタックからポップする（GCの誤動作対策）．
    pop();
    pop();
    push(OBJ_VAL(result));
}

static InterpretResult run() {
    // 現在のフレームをキャッシュ
    CallFrame* frame = &vm.frames[vm.frameCount - 1];

/**
 * マクロ:
 * その場で展開される式ラベルのようなもの．
 * 呼びだしのオーバーヘッドなどがないので，高速だが，
 * 型チェックなどは行われないので，複雑な処理には向かない．
 *
 * 関数:
 * 呼び出しのオーバーヘッドがあるが，型安全でデバッグもしやすい．
 */

/**
 * 現在 ip が指している命令を読みだしてから，ip を１つ次に進める．
 */
#define READ_BYTE() (*frame->ip++)

/**
 * 現在 ip が指している命令を読みだし，
 * その値をインデックスとして持つチャンクの定数プールから，
 * 対応する値を取り出す．
 * （ip を１つ次に進める）
 */
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])

/**
 * チャンクから次の2バイトを取り出して，16ビットの符号なし整数を組み立てる．
 *
 * NOTE: カンマ演算子 `(expr1, expr2)`
 *       評価順序:  左→右。まず expr1 を評価（副作用を完了）し、次に expr2 を評価．
 *       戻り値:    右オペランド expr2 の値（型も expr2 に由来）．
 *
 * e.g. vm.ip[0] = 0xCA, vm.ip[1] = 0x6C の場合
 *      0xCA 0x6C                   vm.ip[0] = 11001010, vm.ip[1] = 01101100
 *      vm.ip += 2                  vm.ip[-2] = 11001010, vm.ip[-1] = 01101100
 *      vm.ip[-2] << 8              11001010 00000000 (8 bit 左にシフト)
 *      vm.ip[-2] << 8 | vm.ip[-1]  11001010 00000000
 *                               OR 00000000 01101100
 *                                = 11001010 01101100
 */
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

/**
 * 定数プールから取得した値を文字列として読みだす．
 */
#define READ_STRING() AS_STRING(READ_CONSTANT())

/**
 * NOTE: プリプロセッサは演算子もテキストトークンとして受け取るので，
 *       引数に渡すことができる．
 * NOTE: マクロは，テキストをコードとしてそのまま展開するだけなので，
 *       do while によって囲うことで，複数の文の実行をどのブロック内でも意図したとおりに実施できるようにしている．
 */
#define BINARY_OP(valueType, op) \
    do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop()); \
        double a = AS_NUMBER(pop()); \
        push(valueType(a op b)); \
    } while (false)

    for (;;) {

#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");

        disassembleInstruction(
            &frame->closure->function->chunk,
            (int)(frame->ip - frame->closure->function->chunk.code)
        );
#endif

        uint8_t instruction;

        // 命令（バイトコード）のデコード（ディスパッチ）を行う．
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_NIL: push(NIL_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_POP: pop(); break;
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                // NOTE: 代入は式なので，必ず値を生成するため，ポップしない（というより，ポップした後プッシュするという動作を省略している．）
                frame->slots[slot] = peek(0);
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING(); // オペランドの読み出し
                Value value;
                if (!tableGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING(); // オペランドの読み出し
                tableSet(&vm.globals, name, peek(0));
                // NOTE: REPL セッションでの利便性維持のため，変数が定義済みかどうかをチェックしない． ＝ 変数の再定義を許容する．
                pop(); // NOTE: ガベージコレクション対策のため，ハッシュ表に値（peek(0)）が追加し終わってからポップする．
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                if (tableSet(&vm.globals, name, peek(0))) {
                    /**
                     * 新規エントリへの追加の場合，変数が未定義ということなので，
                     * 追加したエントリをクリーンアップしたうえで，ランタイムエラーにする．
                     * NOTE: 暗黙の変数宣言は行わない．
                     */
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_COMPILE_ERROR;
                }
                // NOTE: 代入は式なので，大きな式の中にネストしている可能性を考慮して，スタックからポップしない（残しておく）．
                break;
            }
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(0);
                // 代入は式なのでスタックの値はポップせずに残しておく．
                break;
            }
            case OP_GET_PROPERTY: {
                if (!IS_INSTANCE(peek(0))) {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = AS_INSTANCE(peek(0));
                ObjString* name = READ_STRING();

                Value value;
                if (tableGet(&instance->fields, name, &value)) {
                    pop(); // Instance
                    push(value);
                    break;
                }

                // フィールドでなければ，メソッドを探索．
                // warning: フィールドとメソッドで命名が重複していた場合，フィールドが優先される．
                if (!bindMethod(instance->klass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SET_PROPERTY: {
                if (!IS_INSTANCE(peek(1))) {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = AS_INSTANCE(peek(1));
                // note: プロパティが存在しない場合は新規追加されるので，存在チェックは不要．
                tableSet(&instance->fields, READ_STRING(), peek(0));
                Value value = pop();
                pop(); // Instance
                push(value);
                break;
            }
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER:   BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS:      BINARY_OP(BOOL_VAL, <); break;
            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    runtimeError("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBSTRACT: BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY:  BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE:    BINARY_OP(NUMBER_VAL, /); break;
            case OP_NOT:
                push(BOOL_VAL(isFalsey(pop())));
                break;
            case OP_NEGATE:
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            case OP_PRINT: {
                printValue(pop());
                printf("\n");
                // NOTE: 式ではなく文なので，何もプッシュしない（＝ Stack Effect がゼロ）．
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0))) frame->ip += offset;
                /**
                 * NOTE: この命令は，if 文以外の論理演算子でも使われるため，
                 *       式か文かが事前に決まらないので，この時点では条件値をポップしない．
                 * WARNING: つまり，呼び出し元の状況に応じて OP_POP 命令を漏れなく出力する必要がある．
                 */
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            /**
             * NOTE: 関数のパラメータと実際の引数の関連付け
             *       OP_CALL 命令を実行する時点で，
             *       スタック上にはパラメータと同じ順序で引数が配置されているので，
             *       パラメータの個数（Arity）の情報さえあれば正しく処理することができる．
             */
            case OP_CALL: {
                int argCount = READ_BYTE();
                if (!callValue(peek(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                /**
                 * 現在のフレームを，呼び出した関数のフレームに更新
                 *
                 * note: 関数呼び出しの度に現在のフレームを更新していくことで，
                 *       ip も更新され，実質的に命令のジャンプが行える．
                 */
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_INVOKE: {
                ObjString* method = READ_STRING();
                int argCount = READ_BYTE();

                if (!invoke(method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                // 呼び出した関数のフレームにジャンプ．
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_CLOSURE: {
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure* closure = newClosure(function);
                push(OBJ_VAL(closure));

                for (int i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues[i] = captureUpvalue(frame->slots + index);
                    } else {
                        /**
                         * OP_CLOSURE 命令は関数宣言の最後に出力されるので，
                         * 実行時には，
                         * 「参照すべき外側のクロージャ＝現在のCallFrameに格納されているクロージャ」
                         * という状態になっている．
                         */
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }

                break;
            }
            case OP_CLOSE_UPVALUE:
                closeUpvalues(vm.stackTop - 1);
                pop(); // ヒープに移されたので，スタック上の値はもういらない．
                break;
            case OP_RETURN: {
                Value result = pop(); // 一時退避
                closeUpvalues(frame->slots); // 現在の関数が所有するオープン上位値（パラメータや関数内で直接宣言されたローカルなども含む）を全てクローズする．
                vm.frameCount--;

                if (vm.frameCount == 0) {
                    pop(); // トップレベルのスクリプト関数を破棄
                    return INTERPRET_OK;
                }

                vm.stackTop = frame->slots;
                push(result);
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_CLASS:
                push(OBJ_VAL(newClass(READ_STRING())));
                break;
            case OP_METHOD:
                defineMethod(READ_STRING());
                break;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(const char* source) {
    // 簡単のため，コード全体が暗黙の main 関数のようなものにラップされているものとして扱う．
    ObjFunction* function = compile(source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    // GC が勝手にメモリを開放しないように一旦VMのスタックにプッシュする．
    push(OBJ_VAL(function));
    ObjClosure* closure = newClosure(function);
    pop();

    // コンパイラが返した function をスタックスロットの 0 番目にプッシュする．
    // ref. 「NOTE: VM が内部的に利用するローカル変数の予約」
    push(OBJ_VAL(closure));

    call(closure, 0);

    return run();
}
