FROM openjdk:17-slim

WORKDIR /app
COPY src/Hello.java .

RUN javac Hello.java

CMD ["java", "Hello"]
