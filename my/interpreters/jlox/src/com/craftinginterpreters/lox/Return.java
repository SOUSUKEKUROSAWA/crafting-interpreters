package com.craftinginterpreters.lox;

class Return extends RuntimeException {
    final Object value;

    Return(Object value) {
        // NOTE: このクラスはエラー処理ではなく，制御フローのために使うので，
        // スタックトレースなど不要なオーバーヘッドを生むものは無効化する．
        super(null, null, false, false);
        this.value = value;
    }
}
