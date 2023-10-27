FROM alpine:edge AS zenith-alpine

WORKDIR /app

RUN mkdir build 

RUN apk update && apk upgrade

RUN apk add --no-cache \
        alpine-sdk \
        binutils \
        cmake \
        libc-utils \
        libstdc++ \
        llvm-dev \
        llvm15-libs \
        llvm15-static \
        ninja 

COPY ../../* .

RUN cmake -B ./build -G Ninja

RUN cmake --build ./build 

RUN cmake --build ./build --target clean

RUN cmake --install ./build --strip

RUN apk del \
        alpine-sdk \
        binutils \
        cmake \
        libc-utils \
        libstdc++ \
        llvm-dev \
        llvm15-libs \
        llvm15-static \
        ninja 
