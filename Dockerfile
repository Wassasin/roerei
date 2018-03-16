FROM debian:stretch

# Install base system
RUN apt-get -qq update && DEBIAN_FRONTEND=noninteractive apt-get -qq install --no-install-recommends -y \
        curl \
        ca-certificates \
        apt-transport-https \
        sudo \
        gnupg \
        wget \
        patch \
        unzip \
        build-essential \
        git \
        m4 \
        zlib1g-dev \
        clang \
        cmake \
        libboost-chrono-dev \
        libboost-date-time-dev \
        libboost-filesystem-dev \
        libboost-program-options-dev \
        libboost-regex-dev \
        libboost-system-dev \
        libboost-thread-dev \
        libmsgpack-dev \
        check \
    && rm -rf /var/lib/apt/lists/*

RUN wget https://raw.github.com/ocaml/opam/master/shell/opam_installer.sh -O - | sh -s /usr/local/bin

RUN opam init -a --comp=4.01.0 && \
    opam install -y \
        ocamlfind \
        xmlm \
        camlzip \
        extlib \
        msgpack.1.2.1 \
        meta_conv \
        coq.8.4pl4 \
        tiny_json \
        tiny_json_conv \
        treeprint
