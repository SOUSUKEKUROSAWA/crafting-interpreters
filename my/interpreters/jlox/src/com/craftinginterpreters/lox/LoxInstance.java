package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.Map;

class LoxInstance {
    private LoxClass klass;
    private final Map<String, Object> fields = new HashMap<>();

    LoxInstance(LoxClass klass) {
        this.klass = klass;
    }

    Object get(Token name) {
        // WARNING: メソッドより先にフィールドを探す＝フィールドがメソッドを隠す
        if (fields.containsKey(name.lexeme)) {
            return fields.get(name.lexeme);
        }

        // メソッドをダイレクトにコールする場合（e.g. foo.bar();）でも，まず bar を 解釈するために，この処理を通る．
        // つまり，this は必ず bind される．
        LoxFunction method = klass.findMethod(name.lexeme);
        if (method != null) return method.bind(this);

        throw new RuntimeError(name, "Undefined property '" + name.lexeme + "'.");
    }

    void set(Token name, Object value) {
        fields.put(name.lexeme, value);
    }

    @Override
    public String toString() {
        return klass.name + " instance";
    }
}
