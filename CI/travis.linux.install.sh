#!/bin/bash
set -ev
sudo apt-get install \
  build-essential \
  qt5-default qtmultimedia5-dev qttools5-dev \
  libhunspell-dev \
  lua5.1 liblua5.1-0-dev \
  libpcre3-dev \
  libboost-dev \
  zlib1g-dbg zlib1g-dev \
  libzip-dev \
  libpulse-dev \
  cmake
pushd $HOME/yajl
./configure
sudo make install
popd
