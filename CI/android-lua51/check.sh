#!/bin/bash
# Builds the bundled Lua 5.1 for Android x86_64 and checks it does not abort under bionic's FORTIFY, which
# sizes svalue() as 0 bytes at the NDK's default _FORTIFY_SOURCE=2 (see 3rdparty/lua51/CMakeLists.txt).
# Usage: CI/android-lua51/check.sh <android-ndk> <build-dir>
set -euo pipefail

[ $# -eq 2 ] || { echo "usage: $0 <android-ndk> <build-dir>" >&2; exit 2; }
ndk=$1
build=$2
here=$(dirname "$(realpath "$0")")
nm=$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-nm
[ -x "$nm" ] || { echo "$ndk is not an Android NDK: it has no $nm" >&2; exit 1; }
# The runner image chooses the NDK, so a result that changes with no commit can be traced to it
echo "Android NDK $(sed -n 's/^Pkg\.Revision *= *//p' "$ndk/source.properties")"

cmake -S "$here" -B "$build" -DCMAKE_TOOLCHAIN_FILE="$ndk/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=x86_64 -DANDROID_PLATFORM=android-28 -DCMAKE_BUILD_TYPE=Release
cmake --build "$build" --parallel "$(nproc)"

# A __*_chk reference in lgc.c is one of its strchr(svalue(...)) calls, which abort. lstrlib.c's fortified
# calls show that FORTIFY is on at all, without which the lgc.c check would pass for nothing.
undefined=$("$nm" -A --undefined-only "$build/lua51/liblua51.a")
chk_in() { sed -n "s/.*:$1\.o: *U //p" <<< "$undefined" | grep -E '^__.+_chk$' | grep -v '^__stack_chk_' || true; }
[ -n "$(chk_in lstrlib.c)" ] \
    || { echo "lstrlib.c makes no fortified call: is the NDK no longer building with _FORTIFY_SOURCE?" >&2; exit 1; }
found=$(chk_in lgc.c)
[ -z "$found" ] || { printf 'lgc.c makes fortified calls, which abort on Android:\n%s\n' "$found" >&2; exit 1; }

"$build/lua51-openlibs"
