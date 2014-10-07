#!/bin/bash
set -ev
sudo apt-get install \
  build-essential \
  qt5-default qtmultimedia5-dev qttools5-dev \
  libhunspell-dev \
  lua5.1 liblua5.1-0-dev \
  libpcre3-dev \
  libboost1.55-dev\
  zlib1g-dbg zlib1g-dev \
  libzip-dev \
  libpulse-dev \
  cmake \
  gcc-4.7 \
  g++-4.7
sudo update-alternatives --remove gcc /usr/bin/gcc-4.6
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.7 60 --slave /usr/bin/g++ g++ /usr/bin/g++-4.7
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.6 40 --slave /usr/bin/g++ g++ /usr/bin/g++-4.6
pushd $HOME/yajl
./configure
sudo make install
popd
