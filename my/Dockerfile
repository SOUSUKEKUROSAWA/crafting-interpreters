FROM ubuntu:20.04 AS base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    make \
    openjdk-11-jdk \
    curl \
    gnupg \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# DartはUbuntuのリポジトリにはないので、公式のリポジトリを追加する
RUN curl -fsSL https://dl-ssl.google.com/linux/linux_signing_key.pub | apt-key add - \
    && apt-get update \
    && apt-get install -y software-properties-common \
    && add-apt-repository "deb [arch=amd64] https://storage.googleapis.com/download.dartlang.org/linux/debian stable main" \
    && apt-get update \
    && apt-get install -y dart=2.12.0-1 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*
ENV PATH="/usr/lib/dart/bin:${PATH}"

WORKDIR /app

COPY entrypoint.sh .
RUN chmod +x entrypoint.sh

FROM base AS java-build
ENTRYPOINT ["./entrypoint.sh", "-j"]

FROM base AS c-build
ENTRYPOINT ["./entrypoint.sh", "-c"]
