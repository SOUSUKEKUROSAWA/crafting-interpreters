FROM gcc:bookworm

WORKDIR /app
COPY src/main.c .

# コンパイルと実行ファイルの生成
RUN gcc -Wall -Wextra -O2 -o main main.c

# 実行
CMD ["./main"]
