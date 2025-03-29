package com.craftinginterpreters.lox;

class Interpreter imprements Expr.Visitor<Object> {
    @Override
    public Object visitLiteralExpr(Expr.Literal expr) {
        return expr.value;
    }

    @Override
    public Object visitGroupingExpr(Expr.Grouping expr) {
        return evaluate(expr.expression);
    }

    // 渡された Expr クラスに評価を委譲する
    private Object evaluate(Expr expr) {
        return expr.accept(this);
    }
}
