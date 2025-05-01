package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.Map;

class Environment {
    final Environment enclosing; // 親スコープへの参照
    private final Map<String, Object> values = new HashMap<>();

    // グローバルスコープ用
    Environment() {
        enclosing = null;
    }

    // ローカルスコープ用
    Environment(Environment enclosing) {
        this.enclosing = enclosing;
    }

    void define(String name, Object value) {
        // 常に上書きする．
        // REPLではどの変数が宣言済みかを覚えておく必要がない方が楽なので，
        // 宣言済みの変数を再定義することも許容している．
        values.put(name, value);
    }

    Object get(Token name) {
        if (values.containsKey(name.lexeme)) {
            return values.get(name.lexeme);
        }

        // 親スコープを再帰的に探索する
        if (enclosing != null) {
            return enclosing.get(name);
        }

        throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }

    Object getAt(int distance, String name) {
        // NOTE: 変数が存在するかどうかは，Resolver で解決済みなのでチェックしなくてよい．
        return ancestor(distance).values.get(name);
    }

    // 指定された回数分だけ親スコープを遡った環境を返す
    Environment ancestor(int distance) {
        Environment environment = this;
        for (int i = 0; i < distance; i++) {
            environment = environment.enclosing;
        }
        return environment;
    }

    // 代入
    void assign(Token name, Object value) {
        // define と違って，新しい変数の作成は許容しない．
        if (values.containsKey(name.lexeme)) {
            values.put(name.lexeme, value);
            return;
        }

        // 親スコープを再帰的に探索する
        if (enclosing != null) {
            enclosing.assign(name, value);
            return;
        }

        throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }

    void assignAt(int distance, Token name, Object value) {
        ancestor(distance).values.put(name.lexeme, value);
    }
}
