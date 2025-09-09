#include <stdio.h>
#include <stdlib.h>

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

static Parser parser;
Chunk* compilingChunk;
Table stringConstants;

static Chunk* currentChunk() {
    return compilingChunk;
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
 * @param byte1 命令コード（opcode）
 * @param byte2 命令のオペランド（operand）
 */
static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
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

static void endCompiler() {
    emitReturn();

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
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
    ObjString* string = copyString(name->start, name->length);
    Value indexValue;
    if (tableGet(&stringConstants, string, &indexValue)) {
        return (uint8_t)AS_NUMBER(indexValue);
    }

    uint8_t index = makeConstant(OBJ_VAL(string));
    tableSet(&stringConstants, string, NUMBER_VAL((double)index));
    return index;
}

static uint8_t parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);
    return identifierConstant(&parser.previous);
}

static void defineVariable(uint8_t global) {
    emitBytes(OP_DEFINE_GLOBAL, global);
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
    uint8_t arg = identifierConstant(&name);

    if (canAssign && match(TOKEN_EQUAL)) {
        // = が見つかれば，代入式（セッター）としてコンパイルする．
        expression();
        emitBytes(OP_SET_GLOBAL, arg);
    } else {
        emitBytes(OP_GET_GLOBAL, arg);
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
    [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
    [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
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

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
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
    } else {
        expressionStatement();
    }
}

bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    compilingChunk = chunk;
    parser.hadError = false;
    parser.panicMode = false;
    initTable(&stringConstants);

    advance();

    while (!match(TOKEN_EOF)) {
        declaration();
    }

    endCompiler();
    freeTable(&stringConstants);
    return !parser.hadError;
}
