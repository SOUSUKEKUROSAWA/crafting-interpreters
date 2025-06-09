package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

// NOTE: 変数を訪問するたびに，
// 現在のスコープとその変数が定義されたスコープとの間にあるスコープ数をインタプリタに知らせる．
class Resolver implements Expr.Visitor<Void>, Stmt.Visitor<Void> {
    private final Interpreter interpreter;
    private final Stack<Map<String, Boolean>> scopes = new Stack<>();
    private FunctionType currentFunction = FunctionType.NONE;
    private ClassType currentClass = ClassType.NONE;

    private enum FunctionType {
        NONE,
        FUNCTION,
        INITIALIZER,
        METHOD
    }

    private enum ClassType {
        NONE,
        CLASS
    }

    Resolver(Interpreter interpreter) {
        this.interpreter = interpreter;
    }

    void resolve(List<Stmt> statements) {
        for (Stmt statement : statements) {
            resolve(statement);
        }
    }

    private void resolve(Stmt stmt) {
        stmt.accept(this);
    }

    private void resolve(Expr expr) {
        expr.accept(this);
    }

    private void resolveLocal(Expr expr, Token name) {
        // スコープの内から外側へ向かって変数を探索する．
        for (int i = scopes.size() - 1; i >= 0; i--) {
            if (scopes.get(i).containsKey(name.lexeme)) {
                interpreter.resolve(
                    expr,
                    scopes.size() - 1 - i // 最も内側のスコープから，変数を発見するまでに巡ったスコープの数
                );
                return;
            }
        }
    }

    private void resolveFunction(Stmt.Function function, FunctionType type) {
        FunctionType enclosingFunction = currentFunction;
        currentFunction = type;

        beginScope();
        for (Token param : function.params) {
            declare(param);
            define(param);
        }

        // NOTE: 関数宣言のタイミングで，本文の解決も行う（関数呼び出し時ではない）．
        resolve(function.body);
        endScope();

        currentFunction = enclosingFunction;
    }

    private void beginScope() {
        scopes.push(new HashMap<String, Boolean>());
    }

    private void endScope() {
        scopes.pop();
    }

    private void declare(Token name) {
        if (scopes.isEmpty()) return;

        Map<String, Boolean> scope = scopes.peek();

        if (scope.containsKey(name.lexeme)) {
            Lox.error(name, "Already a variable with this name in this scope.");
        }

        scope.put(name.lexeme, false); // false: 宣言されたが初期化されていない
    }

    private void define(Token name) {
        if (scopes.isEmpty()) return;
        scopes.peek().put(name.lexeme, true); // true: 宣言されて初期化もされた
    }

    /* --- 文（Statement） --- */

    // Stack を使って，ブロック単位でスコープを管理する
    @Override
    public Void visitBlockStmt(Stmt.Block stmt) {
        beginScope();
        resolve(stmt.statements);
        endScope();
        return null;
    }

    @Override
    public Void visitClassStmt(Stmt.Class stmt) {
        ClassType enclosingClass = currentClass;
        currentClass = ClassType.CLASS;

        declare(stmt.name);
        define(stmt.name);

        if (
            stmt.superclass != null
            && stmt.name.lexeme.equals(stmt.superclass.name.lexeme)
        ) {
            Lox.error(stmt.superclass.name, "A class cannot inherit from itself.");
        }

        if (stmt.superclass != null) {
            resolve(stmt.superclass);
        }

        // メソッドの本文を解決する前に，それを囲む新しいスコープを設定し，
        // その中に this を変数のように定義しておく
        beginScope();
        scopes.peek().put("this", true);

        for (Stmt.Function method : stmt.methods) {
            FunctionType declaration = FunctionType.METHOD;

            if (method.name.lexeme.equals("init")) {
                declaration = FunctionType.INITIALIZER;
            }

            resolveFunction(method, declaration);
        }

        endScope();

        currentClass = enclosingClass;
        return null;
    }

    // 現在最も内側にあるスコープに変数を追加する
    /*
     * WARNING: 以下のようなケースをエラーにするために，宣言（declare）と定義（define）に分けている．
     * var a = "outer";
     * {
     *   var a = a; // 解釈が分かれてしまうため，エラーにする．
     * }
     */
    @Override
    public Void visitVarStmt(Stmt.Var stmt) {
        declare(stmt.name);
        if (stmt.initializer != null) {
            resolve(stmt.initializer);
        }
        define(stmt.name);
        return null;
    }

    @Override
    public Void visitFunctionStmt(Stmt.Function stmt) {
        declare(stmt.name);
        define(stmt.name);
        // NOTE: 関数本文から自身を呼び出せる（再帰を実現できる）ようにするため，事前に関数名を declare & define する．
        resolveFunction(stmt, FunctionType.FUNCTION);
        return null;
    }

    @Override
    public Void visitExpressionStmt(Stmt.Expression stmt) {
        resolve(stmt.expression);
        return null;
    }

    @Override
    public Void visitIfStmt(Stmt.If stmt) {
        // NOTE: 解決（Resolve）と解釈（Interpret）の違い
        // 制御フローに関係なく，存在するすべての分岐を網羅的に解決する．
        resolve(stmt.condition);
        resolve(stmt.thenBranch);
        if (stmt.elseBranch != null) {
            resolve(stmt.elseBranch);
        }
        return null;
    }

    @Override
    public Void visitPrintStmt(Stmt.Print stmt) {
        resolve(stmt.expression);
        return null;
    }

    @Override
    public Void visitReturnStmt(Stmt.Return stmt) {
        if (currentFunction == FunctionType.NONE) {
            Lox.error(stmt.keyword, "Cannot return from top-level code.");
        }

        if (stmt.value != null) {
            // コンストラクタが，値を返すことを禁止する．
            if (currentFunction == FunctionType.INITIALIZER) {
                Lox.error(stmt.keyword, "Cannot return a value from an initializer.");
            }

            resolve(stmt.value);
        }
        return null;
    }

    @Override
    public Void visitWhileStmt(Stmt.While stmt) {
        resolve(stmt.condition);
        resolve(stmt.body);
        return null;
    }

    /* --- 式（Expression） --- */

    @Override
    public Void visitVariableExpr(Expr.Variable expr) {
        if (!scopes.isEmpty() && scopes.peek().get(expr.name.lexeme) == Boolean.FALSE) {
            Lox.error(expr.name, "Cannot read local variable in its own initializer.");
        }

        resolveLocal(expr, expr.name);
        return null;
    }

    @Override
    public Void visitAssignExpr(Expr.Assign expr) {
        resolve(expr.value);
        resolveLocal(expr, expr.name);
        return null;
    }

    @Override
    public Void visitBinaryExpr(Expr.Binary expr) {
        resolve(expr.left);
        resolve(expr.right);
        return null;
    }

    @Override
    public Void visitCallExpr(Expr.Call expr) {
        resolve(expr.callee);
        for (Expr argument : expr.arguments) {
            resolve(argument);
        }
        return null;
    }

    @Override
    public Void visitGetExpr(Expr.Get expr) {
        resolve(expr.object);
        return null;
    }

    @Override
    public Void visitGroupingExpr(Expr.Grouping expr) {
        resolve(expr.expression);
        return null;
    }

    @Override
    public Void visitLiteralExpr(Expr.Literal expr) {
        // NOTE: リテラル式は，どんな変数も部分式も含まないため，解決すべきものはない．
        return null;
    }

    @Override
    public Void visitLogicalExpr(Expr.Logical expr) {
        resolve(expr.left);
        resolve(expr.right);
        return null;
    }

    @Override
    public Void visitSetExpr(Expr.Set expr) {
        resolve(expr.value);
        resolve(expr.object);
        return null;
    }

    @Override
    public Void visitThisExpr(Expr.This expr) {
        if (currentClass == ClassType.NONE) {
            Lox.error(expr.keyword, "Cannot use 'this' outside of a class.");
            return null;
        }

        resolveLocal(expr, expr.keyword);
        return null;
    }

    @Override
    public Void visitUnaryExpr(Expr.Unary expr) {
        resolve(expr.right);
        return null;
    }
}
