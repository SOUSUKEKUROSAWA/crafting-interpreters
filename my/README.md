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
```

## Grammar

下位ほど優先度高（先に評価される）

```ebnf
program     -> declaration* EOF ;
declaration -> classDecl | traitDecl | funDecl | varDecl | statement ;
classDecl   -> "class" IDENTIFIER ( "<" IDENTIFIER )? "{" function* "}" ;
traitDecl   -> "trait" IDENTIFIER withClause "{" function* "}" ;
withClause  -> ( "with" IDENTIFIER ( "," IDENTIFIER )* )? ;
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
