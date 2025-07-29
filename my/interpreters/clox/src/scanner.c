#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
    const char* start; // 今スキャンしている字句の先頭へのポインタ
    const char* current; // 今見ている文字へのポインタ
    int line; // エラー報告のために現在行を追跡
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

static bool isAtEnd() {
    return *scanner.current == '\0';
}

/**
 * 現在の字句を返し，１つ進む．
 */
static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

/**
 * 現在の字句を返すだけ（進まない）．
 */
static char peek() {
    return *scanner.current;
}

/**
 * 現在位置の次の字句を返すだけ（進まない）．
 */
static char peekNext() {
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}

/**
 * 現在の字句が期待する字句であれば，
 * true を返し，1つ進む．
 * そうでなければ，
 * false を返すだけ（進まない）．
 */
static bool match(char expected) {
    if (isAtEnd()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

static Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static Token errorToken(const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    return token;
}

static void skipWhitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;
                advance();
                break;
            case '/':
                if (peekNext() == '/') {
                    // コメントは行末まで続く
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static Token number() {
    while (isDigit(peek())) advance();

    // 小数のハンドリング
    if (peek() == '.' && isDigit(peekNext())) {
        advance(); // 小数点を消費

        while (isDigit(peek())) advance();
    }

    return makeToken(TOKEN_NUMBER);
}

static Token string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') scanner.line++;
        advance();
    }

    if (isAtEnd()) return errorToken("Unterminated string.");

    advance();
    return makeToken(TOKEN_STRING);
}

Token scanToken() {
    skipWhitespace();
    scanner.start = scanner.current;

    if (isAtEnd()) return makeToken(TOKEN_EOF);

    char c = advance();

    if (isDigit(c)) return number();

    switch (c) {
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '-': return makeToken(TOKEN_MINUS);
        case '+': return makeToken(TOKEN_PLUS);
        case '/': return makeToken(TOKEN_SLASH);
        case '*': return makeToken(TOKEN_STAR);

        case '!': return makeToken(
            match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG
        );
        case '=': return makeToken(
            match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL
        );
        case '<': return makeToken(
            match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS
        );
        case '>': return makeToken(
            match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER
        );
        case '"': return string();

    }

    return errorToken("Unexpected character.");
}
