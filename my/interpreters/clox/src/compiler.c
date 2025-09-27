#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
    Token current; // 現在のトークン
    Token previous; // １つ前のトークン
    bool hadError; // エラーが発生したかどうか
    bool panicMode; // パニックモードに入っているかどうか
} Parser;

/**
 * 解析の優先順位
 * 下に行くほど優先順位高（＝先に評価される）
 */
typedef enum {
    PREC_NONE,          // 優先順位なし（中置演算子でない）
    PREC_ASSIGNMENT,    // =
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,      // == !=
    PREC_COMPARISON,    // < > <= >=
    PREC_TERM,          // + -
    PREC_FACTOR,        // * /
    PREC_UNARY,         // ! -
    PREC_CALL,          // . ()
    PREC_PRIMARY,
} Precedence;

/**
 * 引数に canAssign を取り，何も返さない関数へのポインタ
 */
typedef void (*ParseFn)(bool canAssign);

typedef struct {
    ParseFn prefix; // 与えられたトークン型で始まる前置式（e.g. -a, !a）をコンパイルする関数
    ParseFn infix; // 与えられたトークン型が左オペランドの次に来るような中置式（e.g. a + b, a / b）をコンパイルする関数
    Precedence precedence; // 与えられたトークン型を演算子として使う中置式（e.g. a + b, a / b）の優先順位
} ParseRule;

/**
 * locals 配列内の個々のローカル変数
 */
typedef struct {
    Token name;
    int depth; // このローカル変数を宣言したブロックを囲んでいるブロックの数（scopeDepth）．NOTE: -1 の場合は未初期化状態であることを表す．
} Local;

typedef enum {
    TYPE_FUNCTION, // 関数本文
    TYPE_SCRIPT, // トップレベルコード
} FunctionType;

typedef struct {
    ObjFunction* function; // コンパイラの出力先となる関数オブジェクトへのポインタ．NOTE: 単純化のため，トップレベルコードも暗黙的な関数の中にラップされているものとして扱う．
    FunctionType type; // 対象のコードがトップレベルなのか，関数本文なのかを判別するためのフラグ．

    Local locals[UINT8_COUNT]; // コンパイル時点でスコープに入る全てのローカル変数を格納する配列．NOTE: オペランドが 1 バイトまでと決まっているので，要素数にも固定の上限（UINT8_COUNT）が存在する．
    int localCount; // スコープ内のローカル変数の個数
    int scopeDepth; // 今コンパイルしているコードを囲んでいるブロックの数（e.g. 0 = グローバルスコープ，1 = トップレベルブロック）
} Compiler;

static Parser parser;
Compiler* current = NULL;

static Chunk* currentChunk() {
    return &current->function->chunk;
}

static void errorAt(Token* token, const char* message) {
    if (parser.panicMode) return; // 一度パニックモードに入ったら，それ以降のエラーを無視する．
    parser.panicMode = true;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing to do
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void error(const char* message) {
    errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

/**
 * 現在のトークンを解析し，消費する．
 */
static void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

/**
 * 指定されたトークン型にマッチすれば消費する．
 */
static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

/**
 * @return true: 指定されたトークン型にマッチした（トークンも消費する）．
 *         false: 指定されたトークン型にマッチしなかった（トークンは消費しない）．
 */
static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

/**
 * @param byte 命令コード（opcode）または命令のオペランド（operand）
 */
static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

/**
 * ジャンプが必要な命令のバイトコードを出力する．
 *
 * @return チャンクに出力した命令へのオフセット
 *
 * @note どこまでジャンプするかはコードの先をコンパイルするまでわからないため，
 *       この関数では，プレースホルダで 2 バイト長のオペランドを埋めておくのみ．
 */
static int emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
}

/**
 * @param byte1 命令コード（opcode）
 * @param byte2 命令のオペランド（operand）
 */
static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

/**
 * @param loopStart ループバック先のチャンクにおける位置（バイトコードのオフセット）
 */
static void emitLoop(int loopStart) {
    emitByte(OP_LOOP);

    /**
     * ジャンプ元： currentChunk()->count
     * ジャンプ先： loopStart
     * +2 することで OP_LOOP のオペランドも含めて飛び越えられる．
     */
    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) error("Loop body too large");

    emitByte((offset >> 8) & 0xff); // 上位 8 bit を出力
    emitByte(offset & 0xff); // 下位 8 bit を出力
}

static void emitReturn() {
    emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);

    if (constant > UINT8_MAX) { // UINT8_MAX = uint8_t の最大値 = 255
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

/**
 * JUMP系の命令を実行する際にどのくらいジャンプする必要があるかを計算し，
 * 所与の offset の位置にあるオペランドを置き換える．
 */
static void patchJump(int offset) {
    /**
     * ジャンプする距離の計算
     *
     * ジャンプ元：offset
     * ジャンプ先：currentChunk()->count NOTE: この関数が呼ばれるのは，ジャンプによって着地すべき次の命令を出力する直前なので，ジャンプ先は "現在の" バイトコードの位置になる．
     * NOTE: -2 でジャンプオフセット自身のバイトコードの長さを差し引く．
     */
    int jump = currentChunk()->count - offset -2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    /**
     * ジャンプ距離を表すオペランドを上書きする．
     *
     * e.g. (0x1234 >> 8) & 0xff
     *
     * 1. 0x1234    0001 0010 0011 0100 （2進数表記）
     * 2. >> 8      0000 0000 0001 0010 （＝ 右に8ビット分ずらして0でパディング）
     * 3. & 0xff    0000 0000 0001 0010
     *          AND 0000 0000 1111 1111
     *            = 0000 0000 0001 0010 （＝ 下位8ビットをそのまま取り出す）
     */
    currentChunk()->code[offset] = (jump >> 8) & 0xff; // jump(16 bit) の上位8ビットを取り出してオペランドを上書き．
    currentChunk()->code[offset + 1] = jump & 0xff; // jump(16 bit) の下位8ビットを取り出してオペランドを上書き．
}

static void initCompiler(Compiler* compiler) {
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    current = compiler;
}

static void endCompiler() {
    emitReturn();

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

static void beginScope() {
    current->scopeDepth++;
}

static void endScope() {
    current->scopeDepth--;

    // 今抜け出たローカルスコープ内のローカル変数を全てポップする．
    while (
        current->localCount > 0
        && current->locals[current->localCount - 1].depth > current->scopeDepth
    ) {
        emitByte(OP_POP);
        current->localCount--;
    }
}

/**
 * 再帰的に呼び出される可能性のある関数を前方宣言しておく．
 */

static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

/**
 * 所与の識別子トークンから取り出した字句を，チャンクの定数表に文字列として追加する．
 *
 * @return 識別子が格納された定数表のインデックス
 *
 * @note 文字列全体は，オペランドとしてバイトコードのストリームに含めるには大きすぎるので，
 *       代わりに定数表のインデックスによって参照する．
 */
static uint8_t identifierConstant(Token* name) {
    return makeConstant(
        OBJ_VAL(
            copyString(name->start, name->length)
        )
    );
}

static bool identifierEqual(Token* a, Token* b) {
    if (a->length != b->length) return false; // まず簡易比較
    return memcmp(a->start, b->start, a->length) == 0;
}

/**
 * @return -1:      name に一致する変数が見つからなかった．
 *         -1 以外: name に一致する識別子で，一番最後（最新）に宣言された変数が置かれているインデックス．
 *
 * @note シャドーイング
 *       ローカル変数の配列を後ろから辿っていくことで，一番最後（最新）に宣言された変数のインデックスを返す．
 */
static int resolveLocal(Compiler* compiler, Token* name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifierEqual(name, &local->name)) {
            /**
             * 以下のようなエッジケースをエラーにするための分岐．
             *
             * e.g.
             * {
             *      var a = "outer";
             *      {
             *          var a = a;
             *      }
             * }
             */
            if (local->depth == -1) {
                error("Cant't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

static void addLocal(Token name) {
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variable in function.");
        return;
    }

    Local* local = &current->locals[current->localCount++];

    local->name = name;
    local->depth = -1; // 未初期化状態を表す．
}

/**
 * ローカル変数を「現在のスコープにある変数のリスト」に追加する．
 */
static void declareVariable() {
    /**
     * NOTE: グローバル変数は遅延束縛されるので，
     *       コンパイラでは，どの宣言を見たかの追跡管理は行わない．
     */
    if (current->scopeDepth == 0) return;

    Token* name = &parser.previous;

    // 同じローカルスコープ内での変数の再宣言をエラーにする．
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local* local = &current->locals[i];

        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifierEqual(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    addLocal(*name);
}

static uint8_t parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();

    /**
     * 実行時にはローカルを名前で参照できないので，
     * ローカルスコープ内の宣言ならば，ダミーのインデックスを返す．
     */
    if (current->scopeDepth > 0) return 0;

    return identifierConstant(&parser.previous);
}

static void markInitialized() {
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global) {
    /**
     * NOTE: この時点ですでに変数の値はスタックトップに置かれているため，
     *       ローカルスコープ内であれば，命令コードの出力などを行う必要はない．
     */
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

static void and_(bool canAssign) {
    /**
     * NOTE: AND 短絡の実装
     *       呼び出された時点で左辺の式はすでにコンパイル済み．
     *       つまり，左辺の評価の結果がスタックトップに置かれている．
     *       その値が偽性ならその時点で評価を終了し，右辺の評価をスキップし，
     *       左辺の結果を and 式全体の結果としてスタックに残す．
     */
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    /**
     * 条件値のクリア．
     *
     * ここに処理が来ている時点で左辺は true のため，
     * 右辺の結果が and 式全体の結果と等しくなる．
     * だから，左辺の結果はクリアして問題ない．
     */
    emitByte(OP_POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}

/**
 * WARNING: 左側のオペランド全体がコンパイルされ，
 *          それに続く中置演算子も消費されていることを前提とする．
 */
static void binary(bool canAssign) {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);

    /**
     * NOTE: 2項演算子は左結合なので，右オペランドの優先順位は演算子自体の優先順位よりも1つ高い．
     * e.g. 1+2+3+4 は ((1+2)+3)+4 のように解析される．
     */
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL: emitBytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL: emitByte(OP_EQUAL); break;
        case TOKEN_GREATER: emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS: emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL: emitBytes(OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS: emitByte(OP_ADD); break;
        case TOKEN_MINUS: emitByte(OP_SUBSTRACT); break;
        case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
        default: return;
    }
}

static void literal(bool canAssign) {
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL: emitByte(OP_NIL); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        default: return;
    }
}

/**
 * WARNING: "(" のトークンがすでに消費され，previous に格納されていることを前提とする．
 * NOTE: グルーピング自体に実行時のセマンティクスはないので，バイトコードは出力しない．
 */
static void grouping(bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/**
 * WARNING: 数値リテラルのトークンがすでに消費され，previous に格納されていることを前提とする．
 */
static void number(bool canAssign) {
    double value = strtod(parser.previous.start, NULL); // 文字列を double 型に変換
    emitConstant(NUMBER_VAL(value));
}

static void or_(bool canAssign) {
    /**
     * NOTE: OR 短絡の実装
     *       呼び出された時点で左辺の式はすでにコンパイル済み．
     *       つまり，左辺の評価の結果がスタックトップに置かれている．
     *       その値が偽性なら endJump だけスキップし，右辺の評価に進む．
     *       その値が真性なら endJump に進み，右辺の評価をスキップする．
     *
     * WARNING: OP_JUMP_IF_TRUE のような命令を用意すれば，and_() と同じ処理数で記述できるので，
     *          パフォーマンス観点ではその方が Better だが，命令を増やさずとも実装可能ということを示すために，
     *          あえて，この形で実装している．
     */
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);
    patchJump(elseJump);

    /**
     * 条件値のクリア．
     *
     * ここに処理が来ている時点で左辺は false のため，
     * 右辺の結果が or 式全体の結果と等しくなる．
     * だから，左辺の結果はクリアして問題ない．
     */
    emitByte(OP_POP);
    parsePrecedence(PREC_OR);

    patchJump(endJump);
}

static void string(bool canAssign) {
    emitConstant(OBJ_VAL(
        // 先頭と末尾から引用符（"）を除外するために +1, -2 をする．
        copyString(
            parser.previous.start + 1,
            parser.previous.length - 2
        )
    ));
}

/**
 * @param canAssign true: 現在の優先度が代入が許されるほど低い．
 *        e.g. a * b = c + d; の場合は false．
 */
static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);

    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        // = が見つかれば，代入式（セッター）としてコンパイルする．
        expression();
        /**
         * NOTE: コンパイラにおける locals と，VMが実行時に持つスタックは全く同じレイアウトになるので，
         *       locals のインデックスをそのまま使える．
         */
        emitBytes(setOp, (uint8_t)arg);
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }
}

static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

static void unary(bool canAssign) {
    TokenType operatorType = parser.previous.type;

    parsePrecedence(PREC_UNARY);

    switch (operatorType) {
        case TOKEN_BANG: emitByte(OP_NOT); break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return;
    }
}

/**
 * プラットパーサ全体を駆動する，解析ルール表
 * NOTE: 関数へのポインタを表に記入できるようにするため，それらの関数定義より後に定義している．
 */
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
    [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
    [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
    [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
    [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
    [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
    [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
    [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
    [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
    [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
    [TOKEN_AND]           = {NULL,     and_,   PREC_AND},
    [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
    [TOKEN_OR]            = {NULL,     or_,    PREC_OR},
    [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
    [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

/**
 * 現在のトークンを起点として，引数 precedence 以上の優先順位を持つ式のみ解析する．
 */
static void parsePrecedence(Precedence precedence) {
    /**
     * 前置式の解析
     * NOTE: 第1のトークンは，必ず何らかの前置式に属する．
     */
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }
    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    /**
     * 中置式の解析
     * 次の演算子の優先順位が今の式の優先順位以上なら、その演算子を「今の式の一部」としてパースし続ける
     */
    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    // NOTE: a * b = c + d; のような式をエラーにするための処理．
    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void varDeclaration() {
    uint8_t global = parseVariable("Expect variable name.");

    if (match(TOKEN_EQUAL)) {
        // 明示的な初期化式
        expression();
    } else {
        // 暗黙のうちに nil で初期化
        emitByte(OP_NIL);
    }

    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(global);
}

/**
 * 式文．
 * 式を評価して結果を棄てる．
 *
 * e.g.
 *  1 + 2;
 *  sample();
 */
static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static void forStatement() {
    /**
     * 変数宣言が行われる可能性があるため，
     * スコープで囲む．
     */
    beginScope();

    // 初期化節
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(TOKEN_SEMICOLON)) {
        // 初期化子なし
    } else if (match(TOKEN_VAR)) {
        // 変数宣言
        varDeclaration();
    } else {
        // 式文（＝スタックに何も残さない）
        expressionStatement();
    }

    // 条件節
    int loopStart = currentChunk()->count;
    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
        /**
         * この時点で条件節の結果がスタックトップに置かれている状態．
         * それが false であれば，ジャンプしてループを脱出する．
         */
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // ループバックする前に条件値をクリア
    }

    // インクリメント節
    if (!match(TOKEN_RIGHT_PAREN)) {
        /**
         * NOTE: インクリメント節の，コンパイルタイミングと実行タイミングのずれ問題への対処
         *       コンパイルは今やらないといけないが，
         *       実行するのは本文の実行の後である必要があるので，
         *       一旦無条件で本文までジャンプする．
         */
        int bodyJump = emitJump(OP_JUMP);

        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP); // インクリメント節は副作用のための式なので，結果は棄てる．
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(loopStart); // 条件節へループバック
        loopStart = incrementStart; // ループバック先の上書き

        patchJump(bodyJump);
    }

    // 本文
    statement();
    /**
     * NOTE: インクリメント節の，コンパイルタイミングと実行タイミングのずれ問題への対処 ②
     *       インクリメント節がない場合 → loopStart == loopStart （条件節）にループバック
     *       インクリメント節がある場合 → loopStart == incrementStart （インクリメント節）にループバック
     */
    emitLoop(loopStart);

    // 条件節はオプションなので，まず存在確認を行う．
    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP); // ループを抜ける前に条件値をクリア
    }

    endScope();
}

static void ifStatement() {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression(); // 条件の結果がスタックトップに残るので，それを使って then 節の実行判定を行える．
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after 'if'.");

    /**
     * NOTE: OP_JUMP_IF_FALSE は ip に加えるオフセット（どれだけジャンプするか）をオペランドに取る．
     *       ただし，そのオフセットは then 節をコンパイルするまで分からないため，
     *       バックパッチ（backpatching）というテクニックを使って，一旦プレースホルダ―となるオペランドを出力（emitJump）し，
     *       then 節をコンパイルした後に，実際のオフセットで置き換える（patchJump，後からパッチを当てるニュアンス）という手法を採る．
     */
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // 条件が真性の場合に，スタックに残っている条件値をポップする．
    statement();
    /**
     * then 節を実行を終了した後に，else 節をスキップする．
     *
     * NOTE: 条件に関わらず必ずジャンプするので，OP_JUMP 命令を使う．
     *       then 節が実行されない場合（Falsey だった場合）は，このジャンプ命令自体もスキップされる．
     */
    int elseJump = emitJump(OP_JUMP);
    patchJump(thenJump);
    emitByte(OP_POP); // 条件が偽性の場合に，スタックに残っている条件値をポップする．

    if (match(TOKEN_ELSE)) statement();
    patchJump(elseJump);
}

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

static void whileStatement() {
    int loopStart = currentChunk()->count;

    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP); // 条件値のクリア
    statement();
    emitLoop(loopStart); // ループバック

    patchJump(exitJump);
    emitByte(OP_POP); // 条件値のクリア
}

/**
 * 文の境界となるものに到達するまでトークンを読み捨ててから，
 * パニックモードを抜ける．
 */
static void synchronize() {
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;

        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;

            default: ; // 何もしない．
        }

        advance();
    }
}

static void declaration() {
    if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }

    if (parser.panicMode) synchronize();
}

static void statement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_FOR)) {
        forStatement();
    } else if (match(TOKEN_IF)) {
        ifStatement();
    } else if (match(TOKEN_WHILE)) {
        whileStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler);
    compilingChunk = chunk;
    parser.hadError = false;
    parser.panicMode = false;

    advance();

    while (!match(TOKEN_EOF)) {
        declaration();
    }

    endCompiler();
    return !parser.hadError;
}
