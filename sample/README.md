# Sample of Crafting Interpreters

完成版 CLOX，JLOX での動作確認用のディレクトリ

## Setup

```sh
./run.sh
```

## Usage

```sh
$ ./clox
> print "Hello";
Hello

$ ./jlox
> print "Hello";
Hello

$ echo "print \"Hello\" ;" > sample.lox

$ ./clox sample.lox
Hello

$ ./jlox sample.lox
Hello
```
