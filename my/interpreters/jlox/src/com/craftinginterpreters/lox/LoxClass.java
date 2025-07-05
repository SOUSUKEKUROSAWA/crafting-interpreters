package com.craftinginterpreters.lox;

import java.util.List;
import java.util.Map;

class LoxClass implements LoxCallable {
    final String name;
    private final Map<String, LoxFunction> methods;
    private final LoxClass superclass;

    LoxClass(
        String name,
        LoxClass superclass,
        Map<String, LoxFunction> methods
    ) {
        this.name = name;
        this.superclass = superclass;
        this.methods = methods;
    }

    @Override
    public int arity() {
        LoxInstance instance = new LoxInstance(this);
        LoxFunction initializer = findMethod(instance, "init");
        if (initializer == null) return 0;
        return initializer.arity();
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        LoxInstance instance = new LoxInstance(this);
        LoxFunction initializer = findMethod(instance, "init");

        if (initializer != null) {
            // 即座に束縛することで，コンストラクタ内で this を使えるようにする
            initializer.call(interpreter, arguments);
        }

        return instance;
    }

    @Override
    public String toString() {
        return name;
    }

    LoxFunction findMethod(LoxInstance instance, String name) {
        LoxFunction method = null;
        LoxFunction inner = null;
        LoxClass klass = this;

        // クラスを下から上に辿っていき，inner を上書きしていく
        while (klass != null) {
            if (klass.methods.containsKey(name)) {
                inner = method;
                method = klass.methods.get(name);
            }

            klass = klass.superclass;
        }

        if (method != null) {
            return method.bind(instance, inner);
        }

        return null;
    }
}
