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
```

## Grammar

下位ほど優先度高（先に評価される）

```ebnf
expression  -> comma ;
comma       -> equality ( "," equality )* ;
equality    -> comparison ( ( "!=" | "==" ) comparison )* ; // 左結合（左の演算子が先に評価される）
comparison  -> term ( ( ">" | ">=" | "<" | "<=" ) term )* ; // 左
term        -> factor ( ( "-" | "+" ) factor )* ;           // 左
factor      -> unary ( ( "/" | "*" ) unary )* ;             // 左
unary       -> ( "-" | "!" ) unary | primary ;              // 右
primary     -> NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")" ;
```

※ factor -> factor ( "/" | "*" ) unary | unary ; のようにも書けるが，Parser を実装する際に再帰呼び出しが無限ループに陥るため不可

## Want to add

- リスト／配列
- 例外処理
- ループの break, continue
- switch
