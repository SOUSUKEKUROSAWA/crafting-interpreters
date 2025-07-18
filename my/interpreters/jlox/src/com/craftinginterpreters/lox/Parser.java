package com.craftinginterpreters.lox;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import static com.craftinginterpreters.lox.TokenType.*;

// Expr を継承したクラスのインスタンスを入れ子にしていくことで，AST を構築する．
class Parser {
    private static class ParseError extends RuntimeException {}

    private final List<Token> tokens;
    private int current = 0;

    Parser(List<Token> tokens) {
        this.tokens = tokens;
    }

    List<Stmt> parse() {
        List<Stmt> statements = new ArrayList<>();

        while (!isAtEnd()) {
            statements.add(declaration());
        }

        return statements;
    }

    // declaration -> classDecl | funDecl | varDecl | statement ;
    private Stmt declaration() {
        try {
            if (match(CLASS)) return classDeclaration();
            // funDecl -> "fun" function ;
            if (match(FUN)) return function("function");
            if (match(VAR)) return varDeclaration();
            return statement();
        } catch (ParseError error) {
            synchronize();
            return null;
        }
    }

    // classDecl -> "class" IDENTIFIER ( "<" IDENTIFIER )? "{" function* "}" ;
    private Stmt classDeclaration() {
        // IDENTIFIER
        Token name = consume(IDENTIFIER, "Expect class name.");

        // ( "<" IDENTIFIER )?
        Expr.Variable superclass = null;
        if (match(LESS)) {
            consume(IDENTIFIER, "Expect superclass name.");
            superclass = new Expr.Variable(previous());
        }

        // "{"
        consume(LEFT_BRACE, "Expect '{' before class body.");

        // function*
        List<Stmt.Function> methods = new ArrayList<>();
        while (!check(RIGHT_BRACE) && !isAtEnd()) {
            methods.add(function("method"));
        }

        // "}"
        consume(RIGHT_BRACE, "Expect '}' after class body.");

        return new Stmt.Class(name, superclass, methods);
    }

    // function -> IDENTIFIER "(" parameters? ")" block ;
    // NOTE: kind によって，関数 (function) かメソッド (method) かを判定する．
    private Stmt.Function function(String kind) {
        // IDENTIFIER
        Token name = consume(IDENTIFIER, "Expect " + kind + " name.");

        // "("
        consume(LEFT_PAREN, "Expect '(' after " + kind + " name.");

        // parameters?
        List<Token> parameters = new ArrayList<>();
        if (!check(RIGHT_PAREN)) {
            do {
                if (parameters.size() >= 255) {
                    error(peek(), "Cannot have more than 255 parameters.");
                }
                parameters.add(consume(IDENTIFIER, "Expect parameter name."));
            } while (match(COMMA));
        }

        // ")"
        consume(RIGHT_PAREN, "Expect ')' after parameters.");

        // block
        consume(LEFT_BRACE, "Expect '{' before " + kind + " body.");
        List<Stmt> body = block();

        return new Stmt.Function(name, parameters, body);
    }

    // varDecl -> "var" IDENTIFIER ( "=" expression )? ";" ;
    private Stmt varDeclaration() {
        Token name = consume(IDENTIFIER, "Expect variable name.");

        Expr initializer = null;
        if (match(EQUAL)) {
            initializer = expression();
        }

        consume(SEMICOLON, "Expect ';' after variable declaration.");
        return new Stmt.Var(name, initializer);
    }

    // statement -> exprStmt | forStmt | ifStmt | printStmt | returnStmt | whileStmt | block ;
    private Stmt statement() {
        if (match(FOR)) return forStatement();
        if (match(IF)) return ifStatement();
        if (match(PRINT)) return printStatement();
        if (match(RETURN)) return returnStatement();
        if (match(WHILE)) return whileStatement();
        if (match(LEFT_BRACE)) return new Stmt.Block(block());

        return expressionStatement();
    }

    // forStmt -> "for" "(" (varDecl | exprStmt | ";") expression? ";" expression? ")" statement ;
    private Stmt forStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'for'.");

        // (varDecl | exprStmt | ";")
        Stmt initializer;
        if (match(VAR)) {
            initializer = varDeclaration();
        } else if (match(SEMICOLON)) {
            initializer = null;
        } else {
            initializer = expressionStatement();
        }

        // expression? ";"
        Expr condition = null;
        if (!check(SEMICOLON)) {
            condition = expression();
        }
        consume(SEMICOLON, "Expect ';' after loop condition.");

        // expression? ")"
        Expr increment = null;
        if (!check(RIGHT_PAREN)) {
            increment = expression();
        }
        consume(RIGHT_PAREN, "Expect ')' after for clauses.");

        // statement
        Stmt body = statement();

        /* --- for 文を while 文に desugaring する --- */

        // for (initializer; condition; increment) body;
        // ↓
        // {
        //     initializer;
        //     while (condition) {
        //         body;
        //         increment;
        //     }
        // }

        if (increment != null) {
            body = new Stmt.Block(Arrays.asList(body, new Stmt.Expression(increment)));
        }

        if (condition == null) {
            condition = new Expr.Literal(true);
        }

        body = new Stmt.While(condition, body);

        if (initializer != null) {
            body = new Stmt.Block(Arrays.asList(initializer, body));
        }

        return body;
    }

    // ifStmt -> "if" "(" expression ")" statement ( "else" statement )? ;
    private Stmt ifStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'if'.");
        Expr condition = expression();
        consume(RIGHT_PAREN, "Expect ')' after if condition.");

        Stmt thenBranch = statement();

        // WARNING: else は最も近い先行する if に結びつく．
        // e.g. if (a) if (b) c; else d; // a = true, b = false の場合，d が実行されることを意味する．
        Stmt elseBranch = null;
        if (match(ELSE)) {
            elseBranch = statement();
        }

        return new Stmt.If(condition, thenBranch, elseBranch);
    }

    // whileStmt -> "while" "(" expression ")" statement ;
    private Stmt whileStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'while'.");
        Expr condition = expression();
        consume(RIGHT_PAREN, "Expect ')' after while condition.");

        Stmt body = statement();

        return new Stmt.While(condition, body);
    }

    // block -> "{" declaration* "}" ;
    // WARNING: "{" があることを前提としているので，"{" の消費は呼び出し元の責務になる．
    private List<Stmt> block() {
        List<Stmt> statements = new ArrayList<>();

        while (!check(RIGHT_BRACE) && !isAtEnd()) {
            statements.add(declaration());
        }

        consume(RIGHT_BRACE, "Expect '}' after block.");

        // WARNING: ここで new Stmt.Block を作ると，
        // 関数本文の解析などにこのメソッドを再利用することができなくなってしまうため，
        // 単に文のリストを返すようにしている．
        return statements;
    }

    // printStmt -> "print" expression ";" ;
    private Stmt printStatement() {
        Expr value = expression();
        consume(SEMICOLON, "Expect ';' after value.");
        return new Stmt.Print(value);
    }

    // returnStmt -> "return" expression? ";" ;
    private Stmt returnStatement() {
        Token keyword = previous();

        Expr value = null;
        if (!check(SEMICOLON)) {
            value = expression();
        }

        consume(SEMICOLON, "Expect ';' after return value.");
        return new Stmt.Return(keyword, value);
    }

    // exprStmt -> expression ";" ;
    private Stmt expressionStatement() {
        Expr expr = expression();
        consume(SEMICOLON, "Expect ';' after expression.");
        return new Stmt.Expression(expr);
    }

    // expression -> assignment ;
    // assignment に解析を委譲する．
    private Expr expression() {
        return assignment();
    }

    // assignment  -> ( call "." )? IDENTIFIER "=" assignment | logic_or ;
    // WARNING: "=" を見つけるまで，代入式かどうか（つまり，式として評価して評価結果を返すべきか，疑似式として評価して変数のストレージの場所を調べるべきか）わからない．
    // だが，先読みはトークン１個分しかできないという問題がある．
    // ただ，代入式の左辺値は，変数（IDENTIFIER）だろうが，インスタンスのプロパティ（call.IDENTIFIER）だろうが，必ず式になるので，
    // まずは，式（logic_or）として解析する．
    // "=" に当たれば，その式が代入式であると判断し，疑似式として評価してその変数のストレージの場所を調べる．
    private Expr assignment() {
        Expr expr = or();

        if (match(EQUAL)) {
            Token equals = previous();
            Expr value = assignment();

            // 代入式のノードを作る前に，代入のターゲットが何かを調べる．
            if (expr instanceof Expr.Variable) { // 変数への代入
                Token name = ((Expr.Variable) expr).name;
                return new Expr.Assign(name, value);
            } else if (expr instanceof Expr.Get) { // インスタンスのプロパティへの代入
                Expr.Get get = (Expr.Get) expr;
                return new Expr.Set(get.object, get.name, value);
            }

            // エラーは報告するが，Parse は続行可能なので，throw はしない．
            // e.g. a + b = c; や (a) = 3;
            error(equals, "Invalid assignment target.");
        }

        return expr;
    }

    // logic_or -> logic_and ( "or" logic_and )* ;
    private Expr or() {
        // logic_and
        Expr expr = and();

        // ( "or" logic_and )*
        while (match(OR)) {
            Token operator = previous();
            Expr right = and();
            expr = new Expr.Logical(expr, operator, right);
        }

        return expr;
    }

    // logic_and -> equality ( "and" equality )* ;
    private Expr and() {
        // equality
        Expr expr = equality();

        // ( "and" equality )*
        while (match(AND)) {
            Token operator = previous();
            Expr right = equality();
            expr = new Expr.Logical(expr, operator, right);
        }

        return expr;
    }

    // equality -> comparison ( ( "!=" | "==" ) comparison )* ;
    private Expr equality() {
        // comparison
        Expr expr = comparison();

        // ( ( "!=" | "==" ) comparison )*
        while (match(BANG_EQUAL, EQUAL_EQUAL)) {
            // 演算子がマッチしたら，Binary として expr を更新する．
            Token operator = previous();
            Expr right = comparison();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    // comparison -> term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
    private Expr comparison() {
        // term
        Expr expr = term();

        // ( ( ">" | ">=" | "<" | "<=" ) term )*
        while (match(GREATER, GREATER_EQUAL, LESS, LESS_EQUAL)) {
            // 演算子がマッチしたら，Binary として expr を更新する．
            Token operator = previous();
            Expr right = term();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    // term -> factor ( ( "-" | "+" ) factor )* ;
    private Expr term() {
        // factor
        Expr expr = factor();

        // ( ( "-" | "+" ) factor )*
        while (match(MINUS, PLUS)) {
            // 演算子がマッチしたら，Binary として expr を更新する．
            Token operator = previous();
            Expr right = factor();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    // factor -> unary ( ( "/" | "*" ) unary )* ;
    private Expr factor() {
        // unary
        Expr expr = unary();

        // ( ( "/" | "*" ) unary )*
        while (match(SLASH, STAR)) {
            // 演算子がマッチしたら，Binary として expr を更新する．
            Token operator = previous();
            Expr right = unary();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    // unary -> ( "!" | "-" ) unary | call ;
    private Expr unary() {
        // ( "!" | "-" ) unary
        if (match(BANG, MINUS)) {
            Token operator = previous();
            Expr right = unary();
            return new Expr.Unary(operator, right);
        }

        // call
        return call();
    }

    // call -> primary ( "(" arguments? ")" | "." IDENTIFIER )* ;
    private Expr call() {
        Expr expr = primary();

        while (true) {
            if (match(LEFT_PAREN)) {
                expr = finishCall(expr);
            } else if (match(DOT)) {
                Token name = consume(IDENTIFIER, "Expect property name after '.'.");
                expr = new Expr.Get(expr, name);
            } else {
                break;
            }
        }

        return expr;
    }

    private Expr finishCall(Expr callee) {
        List<Expr> arguments = new ArrayList<>();

        if (!check(RIGHT_PAREN)) {
            do {
                if (arguments.size() >= 255) {
                    // エラーは報告するが，Parse は続行可能なので，throw はしない．
                    error(peek(), "Cannot have more than 255 arguments.");
                }
                arguments.add(expression());
            } while (match(COMMA));
        }

        Token paren = consume(RIGHT_PAREN, "Expect ')' after arguments.");

        return new Expr.Call(callee, paren, arguments);
    }

    // primary -> NUMBER | STRING | "false" | "true" | "nil" | "(" expression ")" | IDENTIFIER | "this" | "super" "." IDENTIFIER ;
    private Expr primary() {
        if (match(FALSE)) return new Expr.Literal(false);
        if (match(TRUE)) return new Expr.Literal(true);
        if (match(NIL)) return new Expr.Literal(null);

        if (match(NUMBER, STRING)) {
            return new Expr.Literal(previous().literal);
        }

        if (match(THIS)) {
            return new Expr.This(previous());
        }

        // "super" "." IDENTIFIER
        if (match(SUPER)) {
            Token keyword = previous();
            consume(DOT, "Expect '.' after 'super'.");
            Token method = consume(IDENTIFIER, "Expect superclass method name.");
            return new Expr.Super(keyword, method);
        }

        if (match(IDENTIFIER)) {
            return new Expr.Variable(previous());
        }

        // "(" expression ")"
        // Grouping を構文木として表現する理由は，代入式の以下２つを区別するため．
        //   a = 3; // OK.
        //   (a) = 3; // Error.
        if (match(LEFT_PAREN)) {
            // Grouping の中身は expression に解析を委譲する（TOPに戻る）．
            Expr expr = expression();
            consume(RIGHT_PAREN, "Expect ')' after expression.");
            return new Expr.Grouping(expr);
        }

        throw error(peek(), "Expect expression.");
    }

    private boolean match(TokenType... types) {
        for (TokenType type : types) {
            if (check(type)) {
                advance();
                return true;
            }
        }

        return false;
    }

    private Token consume(TokenType type, String message) {
        if (check(type)) return advance();

        throw error(peek(), message);
    }

    private boolean check(TokenType type) {
        if (isAtEnd()) return false;
        return peek().type == type;
    }

    private Token advance() {
        if (!isAtEnd()) current++;
        return previous();
    }

    private boolean isAtEnd() {
        return peek().type == EOF;
    }

    private Token peek() {
        return tokens.get(current);
    }

    private Token previous() {
        return tokens.get(current - 1);
    }

    private ParseError error(Token token, String message) {
        Lox.error(token, message);
        return new ParseError();
    }

    // パニックモードに入った後に呼び出し，
    // 次の文の先頭（パニックモードを抜ける位置）までトークンを進める．
    private void synchronize() {
        advance();

        while (!isAtEnd()) {
            if (previous().type == SEMICOLON) return;

            switch (peek().type) {
                case CLASS:
                case FOR:
                case FUN:
                case IF:
                case PRINT:
                case RETURN:
                case VAR:
                case WHILE:
                    return;
            }

            advance();
        }
    }
}
