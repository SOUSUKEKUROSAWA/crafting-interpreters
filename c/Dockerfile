FROM gcc:bookworm

WORKDIR /app
COPY src/hello.c .

# コンパイルと実行ファイルの生成
RUN gcc -Wall -Wextra -O2 -o hello hello.c

# 実行
CMD ["./hello"]
