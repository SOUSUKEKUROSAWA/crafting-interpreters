package com.craftinginterpreters.lox;

enum TokenType {
    // Single-character tokens.
    COMMA,
    DOT,
    LEFT_BRACE,
    LEFT_PAREN,
    MINUS,
    PLUS,
    RIGHT_BRACE,
    RIGHT_PAREN,
    SEMICOLON,
    SLASH,
    STAR,
    QUESTION,
    COLON,
    
    // One or two character tokens.
    BANG,
    BANG_EQUAL,
    EQUAL,
    EQUAL_EQUAL,
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,
    
    // Literals.
    IDENTIFIER,
    NUMBER,
    STRING,
    
    // Keywords.
    AND,
    CLASS,
    ELSE,
    FALSE,
    FOR,
    FUN,
    IF,
    NIL,
    OR,
    PRINT,
    RETURN,
    SUPER,
    THIS,
    TRUE,
    VAR,
    WHILE,
    
    EOF
}
