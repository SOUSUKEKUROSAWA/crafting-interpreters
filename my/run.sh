#!/bin/bash

IMAGE_NAME="my-interpreters"
TAG="1.0"
CHANGED_FILES=$(powershell.exe "git diff --name-only --diff-filter=ACMR" | grep "\.java$" | sed 's|^my/interpreters/jlox/src/||')

echo "Building Docker image..."
docker build -t ${IMAGE_NAME}:${TAG} .

echo "Running container..."
docker run -it --rm \
    -v $(pwd)/interpreters:/app/src \
    -e CHANGED_FILES="$CHANGED_FILES" \
    ${IMAGE_NAME}:${TAG}
