#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

typedef struct {
    Token current; // 現在のトークン
    Token previous; // １つ前のトークン
    bool hadError; // エラーが発生したかどうか
    bool panicMode; // パニックモードに入っているかどうか
} Parser;

/**
 * 解析の優先順位
 * 下に行くほど優先順位高
 */
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,    // =
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,      // == !=
    PREC_COMPARISON,    // < > <= >=
    PREC_TERM,          // + -
    PREC_FACTORY,       // * /
    PREC_UNARY,         // ! -
    PREC_CALL,          // . ()
    PREC_PRIMARY,
} Precedence;

static Parser parser;
Chunk* compilingChunk;

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

static void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
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

    if (constant > UINT8_MAX) {
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
}

/**
 * 現在のトークンを起点として，引数 precedence 以上の優先順位を持つ式のみ解析する．
 */
static void parsePrecedence(Precedence precedence) {
    //
}

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

/**
 * WARNING: "(" のトークンがすでに消費され，previous に格納されていることを前提とする．
 * NOTE: グルーピング自体に実行時のセマンティクスはないので，バイトコードは出力しない．
 */
static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/**
 * WARNING: 数値リテラルのトークンがすでに消費され，previous に格納されていることを前提とする．
 */
static void number() {
    double value = strtod(parser.previous.start, NULL); // 文字列を double 型に変換
    emitConstant(value);
}

static void unary() {
    TokenType operatorType = parser.previous.type;

    parsePrecedence(PREC_UNARY);

    switch (operatorType) {
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return;
    }
}

bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    compilingChunk = chunk;
    parser.hadError = false;
    parser.panicMode = false;

    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    endCompiler();
    return !parser.hadError;
}
