package com.craftinginterpreters.lox;

// Stmt.Var に渡せる形にするために，Expr を継承する必要がある
class Undefined extends Expr {
    public static final Undefined INSTANCE = new Undefined();

    private Undefined() {
        // シングルトン
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
        return null;
    }
}
