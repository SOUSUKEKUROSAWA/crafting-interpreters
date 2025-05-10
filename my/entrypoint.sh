#!/bin/bash

echo "Compiling Java files..."
# WARNING: パッケージ名を正しく解決できるディレクトリまで移動しないとエラーになる
cd /app/src/jlox/src
if [ -n "$CHANGED_FILES" ]; then
  echo "Changed files detected. Compiling only changed files..."
  IFS=$'\n' # WARNING: 空白を含むパスを正しく処理するために、IFSを改行に設定
  for file in $CHANGED_FILES; do
    # WARNING: 日本語コメントを含むので UTF-8 でコンパイルしないとエラーになる
    javac -encoding UTF-8 -d /app/src/jlox/bin $file
    echo "Compiled: $file"
  done
  unset IFS
else
  echo "No changed files detected. Skipping compilation."
fi

# TODO: 必要に応じてビルド手順を追記していく

# エイリアスの登録
echo "Adding aliases..."
echo 'alias jlox="java -cp jlox/bin com.craftinginterpreters.lox.Lox"' >> ~/.bashrc
echo "Added: jlox <=== java -cp jlox/bin com.craftinginterpreters.lox.Lox"
echo 'alias ast="java -cp jlox/bin com.craftinginterpreters.tool.GenerateAst"' >> ~/.bashrc
echo "Added: ast <=== java -cp jlox/bin com.craftinginterpreters.tool.GenerateAst"

# AST の生成
echo "Generating AST..."
cd /app/src
java -cp jlox/bin com.craftinginterpreters.tool.GenerateAst jlox/src/com/craftinginterpreters/lox
echo "AST Generated at jlox/src/com/craftinginterpreters/lox"

cd /app/src
exec bash
