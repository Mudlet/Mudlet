# syntax=docker/dockerfile:1
# Compiles, packages and install-tests Mudlet as an .rpm. Run it through
# CI/linux-packages/build, which passes the source checkout as the build context.

ARG BASE_IMAGE=fedora:44

FROM ${BASE_IMAGE} AS builder

RUN dnf install -y --setopt=install_weak_deps=False \
    ccache cmake file findutils gcc gcc-c++ git make ninja-build patchelf rpm-build \
    compat-lua compat-lua-devel lua5.1-filesystem lua5.1-lpeg luarocks \
    assimp-devel boost-devel hunspell-devel libzip-devel mesa-libGL-devel \
    mesa-libGLU-devel pcre2-devel pugixml-devel sqlite-devel yajl-devel \
    zlib-ng-compat-devel zziplib-devel \
    qt6-qt5compat-devel qt6-qtbase-devel qt6-qtmultimedia-devel qt6-qtspeech-devel \
    qt6-qttools-devel qtkeychain-qt6-devel \
 && dnf clean all

# the ccache mount below repeats uid 2000 because a mount cannot read a build arg
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
COPY --from=packaging --chmod=755 mkrpm.sh /usr/local/bin/
RUN mkrpm.sh

FROM ${BASE_IMAGE} AS verify
COPY --from=packager /build/out /out
COPY --from=packaging --chmod=755 verify-rpm.sh /usr/local/bin/
RUN verify-rpm.sh

FROM scratch AS export-unverified
COPY --from=packager /build/out/ /

FROM scratch AS export
COPY --from=verify /out/ /
