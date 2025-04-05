package com.craftinginterpreters.lox;

import java.util.List;

// Expr の継承（ポリモーフィズム）により，型判定のための分岐をなくす．
// Visitor パターンにより，呼び出し元判定のための分岐をなくす．
// これにより，同じデータ構造に対して，異なるコンテキストで異なる処理を行いたい場合に，データ構造と処理を分離することができる．
// つまり，OOP言語でありながら関数型言語的スタイルに接近することができる．
abstract class Expr {
  abstract <R> R accept(Visitor<R> visitor);

  interface Visitor<R> {
    R visitBinaryExpr(Binary expr);
    R visitGroupingExpr(Grouping expr);
    R visitLiteralExpr(Literal expr);
    R visitUnaryExpr(Unary expr);
  }

  static class Binary extends Expr {
    final Expr left;
    final Token operator;
    final Expr right;

    Binary(Expr left, Token operator, Expr right) {
      this.left = left;
      this.operator = operator;
      this.right = right;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitBinaryExpr(this);
    }
  }

  static class Grouping extends Expr {
    final Expr expression;

    Grouping(Expr expression) {
      this.expression = expression;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitGroupingExpr(this);
    }
  }

  static class Literal extends Expr {
    final Object value;

    Literal(Object value) {
      this.value = value;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitLiteralExpr(this);
    }
  }

  static class Unary extends Expr {
    final Token operator;
    final Expr right;

    Unary(Token operator, Expr right) {
      this.operator = operator;
      this.right = right;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitUnaryExpr(this);
    }
  }

}
