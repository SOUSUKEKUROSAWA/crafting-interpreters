package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

class Resolver implements Expr.Visitor<Void>, Stmt.Visitor<Void> {
    private final Interpreter interpreter;
    private final Stack<Map<String, Boolean>> scopes = new Stack<>();

    Resolver(Interpreter interpreter) {
        this.interpreter = interpreter;
    }

    // Stack を使って，ブロック単位でスコープを管理する
    @Override
    public Void visitBlockStmt(Stmt.Block stmt) {
        beginScope();
        resolve(stmt.statements);
        endScope();
        return null;
    }

    private Void resolve(List<Stmt> statements) {
        for (Stmt statement : statements) {
            resolve(statement);
        }
    }

    private Void resolve(Stmt stmt) {
        stmt.accept(this);
    }

    private Void resolve(Expr expr) {
        expr.accept(this);
    }

    private Void beginScope() {
        scopes.push(new HashMap<String, Boolean>());
    }

    private Void endScope() {
        scopes.pop();
    }

    // 現在最も内側にあるスコープに変数を追加する
    @Override
    public Void visitVarStmt(Stmt.Var stmt) {
        declare(stmt.name);
        if (stmt.initializer != null) {
            resolve(stmt.initializer);
        }
        define(stmt.name);
        return null;
    }

    private Void declare(Token name) {
        if (scopes.isEmpty()) return null;
        Map<String, Boolean> scope = scopes.peek();
        scope.put(name.lexeme, false); // false: 宣言されたが初期化されていない
    }

    private Void define(Token name) {
        if (scopes.isEmpty()) return null;
        scopes.peek().put(name.lexeme, true); // true: 宣言されて初期化もされた
    }
}
