package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.Map;

class Environment {
    private final Map<String, Object> values = new HashMap<>();

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

        throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }

    // 代入
    void assign(Token name, Object value) {
        // define と違って，新しい変数の作成は許容しない．
        if (values.containsKey(name.lexeme)) {
            values.put(name.lexeme, value);
            return;
        }

        throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }
}
