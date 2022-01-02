FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install --no-install-recommends -y \
    build-essential \
    ccache \
    clang-format \
    clang-tidy \
    clazy \
    cmake \
    gdb \
    git \
    libboost-all-dev \
    libboost-dev \
    libglib2.0-dev \
    libglu1-mesa-dev \
    libgstreamer1.0-dev \
    libhunspell-dev \
    liblua5.1-dev \
    libpcre3-dev \
    libpugixml-dev \
    libpulse-dev \
    libqt5opengl5-dev \
    libsecret-1-dev \
    libyajl-dev \
    libzip-dev \
    lua-filesystem \
    lua-rex-pcre \
    lua-sql-sqlite3 \
    lua-zip \
    lua5.1 \
    luarocks \
    mesa-common-dev \
    qt5-default \
    qtmultimedia5-dev \
    qttools5-dev \
    qt5keychain-dev \
    qtcreator \
    valgrind \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

RUN luarocks install lcf && \
    luarocks install luautf8 && \
    luarocks install lua-yajl

# Without this, clang-tidy/format doesn't find the correct c++ headers
ENV CPLUS_INCLUDE_PATH="${CPLUS_INCLUDE_PATH:+${CPLUS_INCLUDE_PATH}:}/usr/lib/gcc/x86_64-linux-gnu/9/include"

RUN useradd -ms /bin/bash mudlet
WORKDIR /home/mudlet

COPY --chown=mudlet:mudlet .clang-tidy CMakeLists.txt CPPLINT.cfg mudlet.desktop mudlet.png mudlet.svg ./
COPY --chown=mudlet:mudlet 3rdparty 3rdparty
COPY --chown=mudlet:mudlet templates templates
COPY --chown=mudlet:mudlet translations translations
COPY --chown=mudlet:mudlet cmake cmake
COPY --chown=mudlet:mudlet test test
COPY --chown=mudlet:mudlet src src

USER mudlet

ARG BUILD_CORES=4
RUN mkdir build && cd build && cmake .. && cmake --build . --parallel ${BUILD_CORES}
RUN mkdir settings

CMD ["build/src/mudlet"]
