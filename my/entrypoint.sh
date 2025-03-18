#!/bin/bash

echo "Building interpreter..."

cd /app/src

# Java ファイルのコンパイル
for file in $(find jlox/src -name "*.java"); do
  javac -d jlox/bin $file
done

# TODO: 必要に応じてビルド手順を追記していく

exec bash
