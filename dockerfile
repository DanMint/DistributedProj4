FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential iputils-ping \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . /app

RUN g++ -std=c++20 -I include src/utils.cpp src/reciever.cpp main.cpp -o /app/main

ENTRYPOINT ["/app/main"]