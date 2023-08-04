FROM alpine:edge AS zenith-alpine

WORKDIR /app

RUN mkdir build 

RUN apk update && apk upgrade

RUN apk add --no-cache \
        clang \
        cmake \
        make \
        musl-dev \
        ninja

COPY ../../* .

RUN cmake -B ./build -G Ninja

RUN cmake --build ./build 

RUN cmake --install ./build --strip

RUN cmake --build ./build --target clean

RUN apk del \
        clang \
        cmake \
        make \
        musl-dev \
        ninja
