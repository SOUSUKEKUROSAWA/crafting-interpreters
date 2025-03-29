#!/bin/bash

echo "Compiling Java files..."
# WARNING: パッケージ名を正しく解決できるディレクトリまで移動しないとエラーになる
cd /app/src/jlox/src
for file in $(find . -name "*.java"); do
  # WARNING: 日本語コメントを含むので UTF-8 でコンパイルしないとエラーになる
  javac -encoding UTF-8 -d /app/src/jlox/bin $file
  echo "Compiled: $file"
done

# TODO: 必要に応じてビルド手順を追記していく

# エイリアスの登録
echo "Adding aliases..."
echo 'alias jlox="java -cp jlox/bin com.craftinginterpreters.lox.Lox"' >> ~/.bashrc
echo "Added: jlox <=== java -cp jlox/bin com.craftinginterpreters.lox.Lox"
echo 'alias ast="java -cp jlox/bin com.craftinginterpreters.tool.GenerateAst"' >> ~/.bashrc
echo "Added: ast <=== java -cp jlox/bin com.craftinginterpreters.tool.GenerateAst"

cd /app/src
exec bash
