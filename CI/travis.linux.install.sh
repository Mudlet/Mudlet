#/bin/bash

mkdir -p "${HOME}/latest-gcc-symlinks"
ln -s /usr/bin/g++-5 "${HOME}/latest-gcc-symlinks/g++"
ln -s /usr/bin/gcc-5 "${HOME}/latest-gcc-symlinks/gcc"
