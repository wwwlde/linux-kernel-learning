FROM ubuntu:12.04
RUN sed -i 's/archive.ubuntu.com/old-releases.ubuntu.com/g' /etc/apt/sources.list && \
    apt-get update && \
    apt-get install -y build-essential wget flex bison curl \
                   make autoconf automake texinfo \
                   gcc-multilib g++-multilib bin86 vim \
                   libc6-dev-i386 ncurses-dev
