
FROM ubuntu:20.04

WORKDIR /ljrServer

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

RUN sed -i s@/ports.ubuntu.com/ubuntu-ports/@/mirrors.ustc.edu.cn/ubuntu-ports/@g /etc/apt/sources.list \
    && sed -i s@/archive.ubuntu.com/@/mirrors.aliyun.com/@g /etc/apt/sources.list \
    && sed -i s@/security.ubuntu.com/@/mirrors.aliyun.com/@g /etc/apt/sources.list \
    && apt-get clean \
    && apt update \
    && apt upgrade -y \
    && apt install gcc g++ gdb git -y \
    && apt install cmake -y \
    && apt install libboost-dev -y \
    && apt install openssh-server -y
