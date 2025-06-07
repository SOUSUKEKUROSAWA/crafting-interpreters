package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.Map;

class LoxInstance {
    private LoxClass klass;
    private final Map<String, Object> fields = new HashMap<>();

    LoxInstance(LoxClass klass) {
        this.klass = klass;
    }

    Object get(Interpreter interpreter, Token name) {
        // WARNING: メソッドより先にフィールドを探す＝フィールドがメソッドを隠す
        if (fields.containsKey(name.lexeme)) {
            return fields.get(name.lexeme);
        }

        LoxFunction getter = klass.findGetter(name.lexeme);
        if (getter != null) {
            // getter が見つかった場合は，それを呼び出して返す．
            return getter.bind(this).call(interpreter, java.util.Collections.emptyList());
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
