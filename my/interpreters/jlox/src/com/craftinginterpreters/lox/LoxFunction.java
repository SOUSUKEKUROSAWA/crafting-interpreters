package com.craftinginterpreters.lox;

import java.util.List;

class LoxFunction implements LoxCallable {
    private final Stmt.Function declaration;
    private final Environment closure;

    LoxFunction(Stmt.Function declaration, Environment closure) {
        this.declaration = declaration;
        this.closure = closure;
    }

    @Override
    public int arity() {
        return declaration.params.size();
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        // NOTE: パラメータをカプセル化するために，関数ごとに新しいローカル環境を用意している．
        Environment environment = new Environment(closure);

        // パラメータと引数を結び付けて，環境に登録する
        for (int i = 0; i < declaration.params.size(); i++) {
            environment.define(declaration.params.get(i).lexeme, arguments.get(i));
        }

        // 用意した環境の中で，関数の本体を実行する
        try {
            interpreter.executeBlock(declaration.body, environment);
        } catch (Return returnValue) {
            // Return 例外をキャッチしたら，その値を返り値とする．
            return returnValue.value;
        }
        return null;
    }

    @Override
    public String toString() {
        return "<fn " + declaration.name.lexeme + ">";
    }

    // メソッドのクロージャの内側にネストした新しい環境を作成し，
    // そこに this を束縛する．
    LoxFunction bind(LoxInstance instance) {
        Environment environment = new Environment(closure);
        environment.define("this", instance);
        return new LoxFunction(declaration, environment);
    }
}
