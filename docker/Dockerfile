# syntax=docker/dockerfile:1

# 26.04 is the first Ubuntu LTS whose own Qt (6.10) meets Mudlet's 6.8.2 minimum
FROM ubuntu:26.04 AS deps

# Keep in line with the "(Linux) Install dependencies" step in .github/workflows/build-mudlet.yml.
# CI takes Qt, Lua 5.1 and luarocks from elsewhere; the qt6-*, lua5.1 and luarocks packages stand in
# for them, as in CI/linux-packages/deb/Dockerfile.
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        ca-certificates \
        ccache \
        clang-format \
        clang-tidy \
        cmake \
        g++ \
        gdb \
        git \
        libassimp-dev \
        libboost-dev \
        libcurl4-openssl-dev \
        libgl1-mesa-dri \
        libglu1-mesa-dev \
        libhunspell-dev \
        liblua5.1-0-dev \
        libonig-dev \
        libpcre2-dev \
        libpugixml-dev \
        libpulse-dev \
        libsecret-1-dev \
        libspeechd-dev \
        libsqlite3-dev \
        libssl-dev \
        libxcb-image0-dev \
        libxcb-render-util0-dev \
        libxkbcommon-x11-0 \
        libyajl-dev \
        libzip-dev \
        lua5.1 \
        luarocks \
        mold \
        ninja-build \
        openssl \
        pcre2-utils \
        pkg-config \
        qt6-5compat-dev \
        qt6-base-dev \
        qt6-httpserver-dev \
        qt6-l10n-tools \
        qt6-multimedia-dev \
        qt6-speech-dev \
        qt6-speech-flite-plugin \
        qt6-svg-dev \
        qt6-tools-dev \
        qt6-websockets-dev \
        qtkeychain-qt6-dev \
        xvfb \
        zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

ENV LANG=C.UTF-8

# UID 1000, held by the image's own "ubuntu" user, usually owns the checkout that gets bind-mounted in
RUN userdel --remove ubuntu \
    && useradd --create-home --uid 1000 --shell /bin/bash mudlet
USER mudlet
WORKDIR /home/mudlet

RUN --mount=type=bind,source=CI/install-lua-rocks.sh,target=/tmp/install-lua-rocks.sh \
    bash /tmp/install-lua-rocks.sh lua-yajl \
    && bash /tmp/install-lua-rocks.sh \
    && bash /tmp/install-lua-rocks.sh --check \
    && mkdir -p /home/mudlet/.cache /home/mudlet/.config/mudlet /home/mudlet/settings

# Mudlet on Linux, unlike macOS, does not search ~/.luarocks, so the --local rocks need the Lua path
ENV LUA_PATH="/home/mudlet/.luarocks/share/lua/5.1/?.lua;/home/mudlet/.luarocks/share/lua/5.1/?/init.lua;;" \
    LUA_CPATH="/home/mudlet/.luarocks/lib/lua/5.1/?.so;;"


FROM deps AS qtcreator

USER root
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        qtcreator \
    && rm -rf /var/lib/apt/lists/*
USER mudlet

WORKDIR /home/mudlet/Mudlet
CMD ["qtcreator", "-settingspath", "/home/mudlet/settings", "CMakeLists.txt"]


FROM deps

WORKDIR /home/mudlet/Mudlet
COPY --chown=mudlet:mudlet . .

ARG BUILD_CORES=4
RUN --mount=type=cache,target=/home/mudlet/.cache/ccache,uid=1000,gid=1000 \
    cmake --preset linux-release -DBUILD_TESTING=OFF \
    && cmake --build --preset linux-release --parallel "${BUILD_CORES}"

CMD ["build-linux-release/src/mudlet"]
