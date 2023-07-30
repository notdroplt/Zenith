FROM ubuntu:devel AS zenith-ubuntu

WORKDIR /app

RUN mkdir build

RUN apt update && apt install -y \
        clang \
        cmake \
        make \
        ninja && \
        rm -rf /var/lib/apt/lists/*

COPY ../../ .

RUN cmake -B ./build -G Ninja

RUN cmake --build ./build --target zenith

RUN cmake --install ./build --strip

RUN cmake --build ./build --target clean

RUN apt purge \
        clang \
        cmake \
        make \
        musl-dev \
        ninja
