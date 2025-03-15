#!/bin/bash

# イメージ名とタグを設定
IMAGE_NAME="main-c"
TAG="1.0"

# Dockerイメージをビルド
echo "Building Docker image..."
docker build -t ${IMAGE_NAME}:${TAG} .

# コンテナを実行
echo "Running container..."
docker run --rm ${IMAGE_NAME}:${TAG}

# 不要になったイメージを削除（オプション）
read -p "Do you want to remove the Docker image? (y/N): " answer
if [[ $answer =~ ^[Yy]$ ]]; then
    echo "Removing Docker image..."
    docker rmi ${IMAGE_NAME}:${TAG}
fi
