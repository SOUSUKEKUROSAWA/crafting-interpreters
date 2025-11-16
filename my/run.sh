#!/bin/bash

IMAGE_NAME="my-interpreters"
TAG="1.0"

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
      echo "  -j: Run jlox"
      echo "  -c: Run clox"
      exit 1
      ;;
  esac
done

if [ "$LANGUAGE" = "java" ]; then
  CHANGED_FILES=$(powershell.exe "git diff --name-only --diff-filter=ACMR" | grep "\.java$" | sed 's|^my/interpreters/jlox/src/||')
elif [ "$LANGUAGE" = "c" ]; then
  CHANGED_FILES=$(powershell.exe "git diff --name-only --diff-filter=ACMR" | grep "\.c$\|\.h$" | sed 's|^my/interpreters/clox/src/||')
else
  echo "Error: 起動モードを選択してください．" >&2
  echo "Usage: $0 [-j|-c]"
  echo "  -j: Run jlox"
  echo "  -c: Run clox"
  exit 1
fi

echo "Building Docker image for $LANGUAGE mode..."
docker build --target ${LANGUAGE}-build -t ${IMAGE_NAME}:${TAG} .

echo "Running container in $LANGUAGE mode..."
docker run -it --rm \
    -v $(pwd)/interpreters:/app/src \
    -e CHANGED_FILES="$CHANGED_FILES" \
    ${IMAGE_NAME}:${TAG}
