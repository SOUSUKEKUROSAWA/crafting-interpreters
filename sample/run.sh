#!/bin/bash

if [ ! -d "craftinginterpreters" ]; then
  echo "Cloning craftinginterpreters repository..."
  git clone https://github.com/munificent/craftinginterpreters.git
else
  echo "Directory 'craftinginterpreters' already exists. Skipping clone."
fi

IMAGE_NAME="craftinginterpreters-sample"
TAG="1.0"

echo "Building Docker image..."
docker build -t ${IMAGE_NAME}:${TAG} .

echo "Running container..."
docker run -it --rm -v $(pwd)/craftinginterpreters:/app/src ${IMAGE_NAME}:${TAG}
