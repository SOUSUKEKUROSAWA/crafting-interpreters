package com.craftinginterpreters.lox;

import java.util.List;

interface LoxCallable {
    int arity(); // arity: 期待する引数の数
    Object call(Interpreter interpreter, List<Object> arguments);
}
