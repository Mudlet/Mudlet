# syntax=docker/dockerfile:1
# Builds, packages and install-tests a .deb; run it via CI/linux-packages/build (source checkout as context).

ARG BASE_IMAGE=ubuntu:26.04

FROM ${BASE_IMAGE} AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    build-essential ca-certificates ccache cmake dpkg-dev file git ninja-build \
    patchelf xz-utils \
    liblua5.1-0-dev lua5.1 lua-filesystem lua-lpeg luarocks \
    libassimp-dev libboost-dev libglu1-mesa-dev libhunspell-dev libpcre2-dev \
    libpugixml-dev libsqlite3-dev libyajl-dev libzip-dev libzzip-dev \
    mesa-common-dev zlib1g-dev \
    libqt6core5compat6-dev libqt6opengl6-dev qt6-base-dev qt6-l10n-tools \
    qt6-multimedia-dev qt6-speech-dev qt6-svg-dev qt6-tools-dev \
    qt6-tools-dev-tools qtkeychain-qt6-dev \
 && rm -rf /var/lib/apt/lists/*

# uid 2000 because Ubuntu images already give 1000 to "ubuntu"; the ccache mount
# below repeats it because a mount cannot read a build arg
RUN useradd --create-home --uid 2000 --user-group builder \
 && mkdir -p /build \
 && chown builder:builder /build
USER builder
WORKDIR /build

COPY --chown=builder:builder . /build/Mudlet

ARG BUILD_COMMIT=
ARG BUILD_JOBS=
ENV CCACHE_DIR=/ccache
ENV WITH_VARIABLE_SPLASH_SCREEN=NO
ENV MUDLET_VERSION_BUILD=""
# apt and dnf update the package, so the built-in updater must not
ENV WITH_UPDATER=NO
RUN --mount=type=cache,target=/ccache,uid=2000,gid=2000 \
    cmake -S Mudlet -B Mudlet/build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -DCMAKE_C_COMPILER_LAUNCHER=ccache \
      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
      -DUSE_SANITIZER="" \
 && ninja -C Mudlet/build ${BUILD_JOBS:+-j"$BUILD_JOBS"} mudlet

FROM builder AS packager

# lrexlib and libssh both export buffer_free; binding lrexlib's own calls locally
# stops libssh's copy freeing a stack TBuffer
RUN luarocks --lua-version 5.1 --tree=/build/rocks install lrexlib-pcre2 \
      LIBFLAG="-shared -Wl,-Bsymbolic-functions" \
 && for rock in luautf8 lua-yajl luazip; do \
      luarocks --lua-version 5.1 --tree=/build/rocks install "$rock" || exit 1; \
    done
# db:create fails on luasql-sqlite3 2.7 and newer, so every Mudlet build pins it
RUN luarocks --lua-version 5.1 --tree=/build/rocks install luasql-sqlite3 2.6.1

ARG BUILD_COMMIT=
ARG PKG_VERSION
ARG PKG_SNAPSHOT=
ARG PKG_RELEASE=1
COPY --from=packaging --chmod=755 mkdeb.sh /usr/local/bin/
RUN mkdeb.sh

FROM ${BASE_IMAGE} AS verify
ENV DEBIAN_FRONTEND=noninteractive
COPY --from=packager /build/out /out
COPY --from=packaging --chmod=755 verify-deb.sh /usr/local/bin/
RUN verify-deb.sh

FROM scratch AS export-unverified
COPY --from=packager /build/out/ /

FROM scratch AS export
COPY --from=verify /out/ /
