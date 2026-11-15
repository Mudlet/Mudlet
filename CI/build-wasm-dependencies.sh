#!/bin/bash
# Builds the libraries the WebAssembly build of Mudlet and its bundled Lua C modules
# link against, as static libraries, into the prefix given as the only argument:
#
#   source /path/to/emsdk/emsdk_env.sh
#   CI/build-wasm-dependencies.sh "$HOME/mudlet-wasm-deps"
#
# Each source is pinned and checked against its sha256 (yajl, which publishes no
# release archive, against its commit). A library already built into the prefix by
# this same script, yajl patch and emcc is skipped, so rerunning it is cheap.

set -euo pipefail

if [ $# -ne 1 ] || [ -z "$1" ]; then
  echo "usage: $0 <install prefix>" >&2
  exit 2
fi
STAMP_SUBDIR=share/mudlet-wasm-deps

# Nothing else may share it: a desktop build may search a prefix such as ~/.local and find these
if [ -d "$1" ] && [ -n "$(ls -A "$1")" ] && [ ! -d "$1/$STAMP_SUBDIR" ]; then
  echo "$1 holds something this script did not build; give it a new or empty directory" >&2
  exit 2
fi

for tool in emcc curl git ninja make cmake; do
  if ! command -v "$tool" > /dev/null; then
    echo "$tool not found (for emcc, source emsdk_env.sh)" >&2
    exit 1
  fi
done

mkdir -p "$1/$STAMP_SUBDIR" "$1/include" "$1/lib"
PREFIX="$(cd "$1" && pwd)"
STAMPS="$PREFIX/$STAMP_SUBDIR"
YAJL_PATCH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/yajl-cmake-fix.patch"
JOBS=4

WORK="$(mktemp -d)"

# Each library's output goes to a log of its own, which is shown if it fails to build
exec 3>&1 4>&2
LIBRARY=""
finish() {
  local status=$?
  exec >&3 2>&4
  if [ "$status" -ne 0 ] && [ -n "$LIBRARY" ]; then
    echo "building $LIBRARY failed; its log:" >&2
    cat "$WORK/$LIBRARY.log" >&2
    # zlib's configure prints only that it gave up; why is in its configure.log, about to be deleted
    if [ "$LIBRARY" = zlib ] && [ -f "$WORK/zlib-$ZLIB_VERSION/configure.log" ]; then
      echo "the end of its configure.log:" >&2
      tail -n 30 "$WORK/zlib-$ZLIB_VERSION/configure.log" >&2
    fi
  fi
  rm -rf "$WORK"
}
trap finish EXIT

# needs_build <name>: fails if the prefix holds <name> built with this $BUILD_ID, else drops its stamps first (an install
# cut short leaves files replaced); it runs as an if condition, where set -e does not apply, so each step exits itself
needs_build() {
  if [ "$(cat "$STAMPS/$1" 2> /dev/null)" = "$BUILD_ID" ]; then
    return 1
  fi
  rm -f "$STAMPS/complete" "$STAMPS/$1" || exit 1
  exec > "$WORK/$1.log" 2>&1 || exit 1
  LIBRARY="$1"
}

sha256() {
  if command -v sha256sum > /dev/null; then
    sha256sum "$1"
  else
    shasum -a 256 "$1"
  fi | cut -d' ' -f1
}

BUILD_ID="emcc-$(emcc -dumpversion) script-$(sha256 "${BASH_SOURCE[0]}") yajl-patch-$(sha256 "$YAJL_PATCH")"

# fetch <url> <sha256>: downloads into $WORK, verifies it and unpacks it there
fetch() {
  local archive
  archive="$WORK/$(basename "$1")"
  # Bounded, as a server that accepts and then sends nothing would hold curl until the job's timeout
  curl -fsSL --retry 3 --connect-timeout 30 --speed-limit 1024 --speed-time 60 -o "$archive" "$1"
  if [ "$(sha256 "$archive")" != "$2" ]; then
    echo "checksum mismatch for $1" >&2
    exit 1
  fi
  tar -xf "$archive" -C "$WORK"
  rm "$archive"
}

stamp() {
  exec >&3 2>&4
  echo "$BUILD_ID" > "$STAMPS/$LIBRARY"
  echo "built $LIBRARY"
  LIBRARY=""
}

cmake_install() {
  local src="$1"
  shift
  emcmake cmake -S "$src" -B "$src/build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" -DBUILD_SHARED_LIBS=OFF "$@"
  cmake --build "$src/build" -j "$JOBS"
  cmake --install "$src/build"
}

# emar adds to an existing archive, so a rebuild starts from none
archive() {
  local library="$PREFIX/lib/$1"
  shift
  rm -f "$library"
  emar rcs "$library" "$@"
}

ZLIB_VERSION=1.3.1
if needs_build zlib; then
  fetch "https://github.com/madler/zlib/releases/download/v$ZLIB_VERSION/zlib-$ZLIB_VERSION.tar.gz" \
    9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23
  (
    cd "$WORK/zlib-$ZLIB_VERSION"
    emconfigure ./configure --static --prefix="$PREFIX"
    emmake make -j "$JOBS" libz.a
    emmake make install
  )
  stamp
fi

# The version Qt bundles (6.9.0: 10.45), which keeps _pcre2_ckd_smul, the one global function
# Mudlet's copy and Qt's both define, identical; the link takes Qt's (see src/CMakeLists.txt)
PCRE2_VERSION=10.45
if needs_build pcre2; then
  fetch "https://github.com/PCRE2Project/pcre2/releases/download/pcre2-$PCRE2_VERSION/pcre2-$PCRE2_VERSION.tar.bz2" \
    21547f3516120c75597e5b30a992e27a592a31950b5140e7b8bfde3f192033c4
  cmake_install "$WORK/pcre2-$PCRE2_VERSION" \
    -DPCRE2_BUILD_PCRE2_8=ON -DPCRE2_BUILD_PCRE2_16=OFF -DPCRE2_BUILD_PCRE2_32=OFF \
    -DPCRE2_SUPPORT_JIT=OFF -DPCRE2_SUPPORT_UNICODE=ON \
    -DPCRE2_BUILD_PCRE2GREP=OFF -DPCRE2_BUILD_TESTS=OFF
  stamp
fi

LIBZIP_VERSION=1.10.1
if needs_build libzip; then
  fetch "https://github.com/nih-at/libzip/releases/download/v$LIBZIP_VERSION/libzip-$LIBZIP_VERSION.tar.xz" \
    dc3c8d5b4c8bbd09626864f6bcf93de701540f761d76b85d7c7d710f4bd90318
  cmake_install "$WORK/libzip-$LIBZIP_VERSION" \
    -DZLIB_INCLUDE_DIR="$PREFIX/include" -DZLIB_LIBRARY="$PREFIX/lib/libz.a" \
    -DBUILD_TOOLS=OFF -DBUILD_REGRESS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_DOC=OFF \
    -DENABLE_BZIP2=OFF -DENABLE_LZMA=OFF -DENABLE_ZSTD=OFF \
    -DENABLE_COMMONCRYPTO=OFF -DENABLE_GNUTLS=OFF -DENABLE_MBEDTLS=OFF -DENABLE_OPENSSL=OFF \
    -DENABLE_WINDOWS_CRYPTO=OFF
  stamp
fi

PUGIXML_VERSION=1.14
if needs_build pugixml; then
  fetch "https://github.com/zeux/pugixml/releases/download/v$PUGIXML_VERSION/pugixml-$PUGIXML_VERSION.tar.gz" \
    2f10e276870c64b1db6809050a75e11a897a8d7456c4be5c6b2e35a11168a015
  cmake_install "$WORK/pugixml-$PUGIXML_VERSION"
  stamp
fi

# The commit yajl 2.1.0 was released from
YAJL_COMMIT=a0ecdde0c042b9256170f2f8890dd9451a4240aa
if needs_build yajl; then
  git init -q "$WORK/yajl"
  git -C "$WORK/yajl" -c http.lowSpeedLimit=1024 -c http.lowSpeedTime=60 \
    fetch -q --depth 1 https://github.com/lloyd/yajl.git "$YAJL_COMMIT"
  git -C "$WORK/yajl" checkout -q FETCH_HEAD
  # As the Flatpak build does: the patch and the policy floor let CMake 4 read yajl's build
  git -C "$WORK/yajl" apply "$YAJL_PATCH"
  cmake_install "$WORK/yajl" -DCMAKE_POLICY_VERSION_MINIMUM=3.5
  stamp
fi

# For LuaSQL; the amalgamation is one file, built without threads as Qt for WebAssembly has none
SQLITE_VERSION=3460100
if needs_build sqlite; then
  fetch "https://www.sqlite.org/2024/sqlite-autoconf-$SQLITE_VERSION.tar.gz" \
    67d3fe6d268e6eaddcae3727fce58fcc8e9c53869bdd07a0c61e38ddf2965071
  (
    cd "$WORK/sqlite-autoconf-$SQLITE_VERSION"
    emcc -O2 -DSQLITE_THREADSAFE=0 -c sqlite3.c -o sqlite3.o
    archive libsqlite3.a sqlite3.o
    cp sqlite3.h sqlite3ext.h "$PREFIX/include/"
  )
  stamp
fi

HUNSPELL_VERSION=1.7.2
if needs_build hunspell; then
  fetch "https://github.com/hunspell/hunspell/releases/download/v$HUNSPELL_VERSION/hunspell-$HUNSPELL_VERSION.tar.gz" \
    11ddfa39afe28c28539fe65fc4f1592d410c1e9b6dd7d8a91ca25d85e9ec65b8
  (
    cd "$WORK/hunspell-$HUNSPELL_VERSION/src/hunspell"
    sed 's/@HAVE_VISIBILITY@/1/' hunvisapi.h.in > hunvisapi.h
    mkdir -p "$PREFIX/include/hunspell" objs
    for f in affentry affixmgr csutil filemgr hashmgr hunspell hunzip phonet replist suggestmgr; do
      em++ -O2 -std=c++11 -fvisibility=hidden -DBUILDING_LIBHUNSPELL -c "$f.cxx" -o "objs/$f.o"
    done
    archive libhunspell.a objs/*.o
    cp hunspell.h hunspell.hxx hunvisapi.h w_char.hxx atypes.hxx "$PREFIX/include/hunspell/"
  )
  stamp
fi

# Mudlet uses only header-only Boost libraries; b2 installs them along with the CMake
# package files that find_package(Boost CONFIG) reads, compiling nothing for wasm
BOOST_VERSION=1.83.0
if needs_build boost; then
  BOOST_DIR="boost_${BOOST_VERSION//./_}"
  fetch "https://archives.boost.io/release/$BOOST_VERSION/source/$BOOST_DIR.tar.bz2" \
    6478edfe2f3305127cffe8caf73ea0176c53769f4bf1585be237eb30798c3b8e
  (
    cd "$WORK/$BOOST_DIR"
    ./bootstrap.sh
    ./b2 -q -d0 --with-headers --prefix="$PREFIX" install
  )
  stamp
fi

# Last, so that only a run that built every library leaves it; CMakeLists.txt reads this stamp alone
echo "$BUILD_ID" > "$STAMPS/complete"
