#!/bin/bash

LANGUAGE=""
while getopts "jc" opt; do
  case $opt in
    j)
      LANGUAGE="java"
      ;;
    c)
      LANGUAGE="c"
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      echo "Usage: $0 [-j|-c]"
      echo "  -j: Java mode"
      echo "  -c: C mode"
      exit 1
      ;;
  esac
done

if [ "$LANGUAGE" = "java" ]; then
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
    echo "No changed files detected. Compiling all files..."
    for file in $(find . -name "*.java"); do
      javac -encoding UTF-8 -d /app/src/jlox/bin $file
      echo "Compiled: $file"
    done
  fi

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
elif [ "$LANGUAGE" = "c" ]; then
  cd /app/src/clox/src

  echo "Compiling all files..."
  # すべての.cファイルをコンパイル
  for file in $(find . -name "*.c"); do
    obj_file="/app/src/clox/bin/${file%.c}.o"
    if ! gcc -Werror -c -o "$obj_file" "$file"; then
      echo "Error: Failed to compile $file" >&2
      echo "Compilation aborted due to error." >&2
      exit 1
    fi
    echo "Compiled: $file -> $obj_file"
  done

  # 最終的な実行ファイルをリンク
  echo "Linking executable..."
  cd /app/src/clox/bin
  gcc -o clox *.o
  echo "Created executable: /app/src/clox/bin/clox"

  # エイリアスの登録
  echo "Adding aliases..."
  echo 'alias clox="/app/src/clox/bin/clox"' >> ~/.bashrc
  echo "Added: clox <=== /app/src/clox/bin/clox"

  cd /app/src
  exec bash
else
  echo "Error: 起動モードを選択してください．" >&2
  echo "Usage: $0 [-j|-c]"
  echo "  -j: Java mode"
  echo "  -c: C mode"
  exit 1
fi
