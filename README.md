# Crafting Interpreters

<https://craftinginterpreters.com/repo>

## Challenge

### 2章

- PHP scanner
  - <https://github.com/php/php-src/blob/master/Zend/zend_language_scanner.l>
- PHP parser
  - <https://github.com/php/php-src/blob/master/Zend/zend_language_parser.y>

### 5章

```ebnf
expr -> expr ( "(" ( expr ( "," expr )* )? ")" | "." IDENTIFIER )+ | IDENTIFIER | NUMBER ;
```

```ebnf
expr -> expr calls ;
expr -> IDENTIFIER ;
expr -> NUMBER ;

calls -> call ;
calls -> calls call ;

call -> "(" ")" ;
call -> "(" arguments ")" ;
call -> "." IDENTIFIER ;

arguments -> expr ;
arguments -> arguments "," expr ;
```

## LSP (Language Server Protocol)

<https://marketplace.visualstudio.com/items?itemName=FabianJakobs.lox-lsp>

## Bug Report

1. メソッド名とフィールド名が重複していた場合の挙動

  ```lox
  class Brunch {
      init(drink) {
          this.drink = drink;
      }

      drink() {
          print "How about a " + this.drink + "?";
      }
  }

  var brunch = Brunch("coffee");
  brunch.drink(); // expected: "How about a coffee?", actual: Can only call functions and classes.
  ```

  メソッド名とフィールド名の重複を解消すればエラーも解消する．

  ```lox
  class Brunch {
      init(drink) {
          this.drink = drink;
      }

      recommend() {
          print "How about a " + this.drink + "?";
      }
  }

  var brunch = Brunch("coffee");
  brunch.recommend(); // "How about a coffee?"
  ```
