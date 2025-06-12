FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    build-essential \
    nasm \
    qemu-system-x86 \
    gdb \
    clang-format \
    tcpdump \
    dnsmasq \
    bridge-utils \
    iproute2 \
    net-tools \
    procps \
    mtools dosfstools \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace