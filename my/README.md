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
program     -> statement* EOF ;
statement   -> exprStmt | printStmt ;
exprStmt    -> expression ";" ;
printStmt   -> "print" expression ";" ;
expression  -> equality ;
equality    -> comparison ( ( "!=" | "==" ) comparison )* ; // 左結合（左の演算子が先に評価される）
comparison  -> term ( ( ">" | ">=" | "<" | "<=" ) term )* ; // 左
term        -> factor ( ( "-" | "+" ) factor )* ;           // 左
factor      -> unary ( ( "/" | "*" ) unary )* ;             // 左
unary       -> ( "-" | "!" ) unary | primary ;              // 右
primary     -> NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")" ;
```

※ factor -> factor ( "/" | "*" ) unary | unary ; のようにも書けるが，Parser を実装する際に再帰呼び出しが無限ループに陥るため不可
※ 式と文の違い＝ExprクラスとStmtクラスの違い（式＝評価の結果を返す．文＝評価の結果を返さない．）

## Types

| Lox の型 | Java による表現 |
| - | - |
| 全ての Lox 値 | Object |
| nil | null |
| ブール型 | Boolean |
| 数値型 | Double |
| 文字列型 | String |

## Want to add

- リスト／配列
- 例外処理
- ループの break, continue
- switch
