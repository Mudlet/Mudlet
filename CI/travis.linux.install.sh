#!/bin/bash

mkdir -p "${HOME}/latest-gcc-symlinks"
ln -s /usr/bin/g++-5 "${HOME}/latest-gcc-symlinks/g++"
ln -s /usr/bin/gcc-5 "${HOME}/latest-gcc-symlinks/gcc"

# lua-utf8 is not in the repositories...
luarocks install --local luautf8


