# My Interpreters

自作 JLOX, CLOX での動作確認用ディレクトリ

JLOX

- Java で書かれた「木の巡回による」インタプリタ
- Scanner -- Tokens --> Parser (Frontend) -- Syntax Trees --> Interpreter (Backend) で実行
- JVM に依存する分，面倒を見る必要のある部分が少ない（簡潔な実装で済む）
- Syntax Trees はメモリ空間上に散らばっているため，キャッシュ効率が悪く，情報量も多い
- 実行速度は低速

CLOX

- C で書かれた「バイトコード式」インタプリタ
- Scanner -- Tokens --> Compiler (Frontend) -- Bytecode --> Virtual Machine (Backend) で実行
- 面倒を見る必要のある部分が多い
- Bytecode はメモリ空間上に密に連続して並ぶため，キャッシュ効率が高く，情報量も少ない
- 実行速度は高速

## Setup

```sh
./run.sh -j # JLOX の実行環境をセットアップ
./run.sh -c # CLOX の実行環境をセットアップ
```

## Usage

```sh
# Run Prompt
jlox

# Run File
jlox test/sample.lox
```

```sh
# Run Prompt
clox

# Run File
clox test/sample.lox
```

## Profiling

```sh
# Profiling
# warning: clox のエイリアスは Bash 上で設定しただけなので，ここでは絶対パス指定が必要．
perf record -g /app/src/clox/bin/clox /app/src/test/sample.lox

# Show Result
# note: 気になった行で[a]ボタンを押すと Annotate モードになり，関数内の詳細な負荷割合を閲覧できる．
# note: Annotate モード内で[h]ボタンを押すと，コマンドヘルプを確認できる．
perf report -f
```

## Grammar

下位ほど優先度高（先に評価される）

```ebnf
program     -> declaration* EOF ;
declaration -> classDecl | funDecl | varDecl | statement ;
classDecl   -> "class" IDENTIFIER ( "<" IDENTIFIER )? "{" function* "}" ;
funDecl     -> "fun" function ;
function    -> IDENTIFIER "(" parameters? ")" block ;
parameters  -> IDENTIFIER ( "," IDENTIFIER )* ;
varDecl     -> "var" IDENTIFIER ( "=" expression )? ";" ;
statement   -> exprStmt | forStmt | ifStmt | printStmt | returnStmt | whileStmt | block ;
exprStmt    -> expression ";" ;
forStmt     -> "for" "(" ( varDecl | exprStmt | ";" ) expression? ";" expression? ")" statement ;
ifStmt      -> "if" "(" expression ")" statement ( "else" statement )? ;
printStmt   -> "print" expression ";" ;
returnStmt  -> "return" expression? ";" ;
whileStmt   -> "while" "(" expression ")" statement ;
block       -> "{" declaration* "}" ;
expression  -> assignment ;
assignment  -> ( call "." )? IDENTIFIER "=" assignment | logic_or ; // 右
logic_or    -> logic_and ( "or" logic_and )* ;                      // 左
logic_and   -> equality ( "and" equality )* ;                       // 左
equality    -> comparison ( ( "!=" | "==" ) comparison )* ;         // 左結合
comparison  -> term ( ( ">" | ">=" | "<" | "<=" ) term )* ;         // 左
term        -> factor ( ( "-" | "+" ) factor )* ;                   // 左
factor      -> unary ( ( "/" | "*" ) unary )* ;                     // 左
unary       -> ( "-" | "!" ) unary | call ;                         // 右
call        -> primary ( "(" arguments? ")" | "." IDENTIFIER )* ;   // 左
arguments   -> expression ( "," expression )* ;                     // 左
primary     -> NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")" | IDENTIFIER | "this" | "super" "." IDENTIFIER ;
```

※ 左結合: 左オペランドが先に評価される
※ factor -> factor ( "/" | "*" ) unary | unary ; のようにも書けるが，Parser を実装する際に再帰呼び出しが無限ループに陥るため不可
※ 式(expression)と文(statement)の違い＝ExprクラスとStmtクラスの違い（式＝評価の結果を返す．文＝評価の結果を返さない．）
※ 宣言と文は使える場所が異なるので区別している（e.g. OK: if (monday) print "bagel";, NG: if (monday) var breakfast = "bagel";）
※ return; は return nil; と同義
※ super は this のように単体では使えない．

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

## Want to make

普通の英文みたいに書けるプログラミング言語

e.g.

```txt
i add 3 and 4, for 6 times, if x equal to 7.
if i eat this cake, i pay 3 dollars.
```
