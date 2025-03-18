#!/bin/bash

IMAGE_NAME="my-interpreters"
TAG="1.0"

echo "Building Docker image..."
docker build -t ${IMAGE_NAME}:${TAG} .

echo "Running container..."
docker run -it --rm -v $(pwd)/interpreters:/app/src ${IMAGE_NAME}:${TAG}
