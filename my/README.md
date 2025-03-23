# My Interpreters

自作 JLOX での動作確認用ディレクトリ

## Setup

```sh
./run.sh
```

## Usage

```sh
# Run Prompt
jlox

# Run File
jlox jlox/sample.lox

# Generate Ast
ast jlox/src/com/craftinginterpreters/lox

# Print Ast
past
```

## Grammar

```ebnf
expression  -> literal | unary | binary | grouping ;
literal     -> NUMBER | STRING | "true" | "false" | "nil" ;
unary       -> ( "-" | "!" ) expression ;
binary      -> expression operator expression
grouping    -> "(" expression ")" ;
operator    -> "==" | "!=" | "<" | "<=" | ">" | ">=" | "+" | "-" | "*" | "/"
```

## Want to add

- リスト／配列
- 例外処理
- ループの break, continue
- switch
