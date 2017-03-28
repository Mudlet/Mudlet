#!/bin/bash
set -ev
# remove cmake and cmake-data that comes with precise by default on travis, as we'd like to upgrade it to cmake from ppa (which does not use a corresponding cmake-data)
sudo apt-get remove \
  qt4-qmake libqt4-designer libqt4-dev libboost-dev cmake cmake-data
sudo apt-get autoremove
# libboost seems messed up in Precise 12.04 lts it mixes 1.46 and 1.48 so try
# and force a uniform 1.55 installation
sudo apt-get install \
  build-essential \
  qt5-default qtmultimedia5-dev qttools5-dev \
  libhunspell-dev \
  lua5.1 liblua5.1-0-dev \
  libpcre3-dev \
  libboost1.55-dev \
  zlib1g-dbg zlib1g-dev \
  libzip-dev \
  libpulse-dev \
  cmake cmake-data \
  gcc-4.7 \
  g++-4.7
sudo update-alternatives --remove gcc /usr/bin/gcc-4.6
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.7 60 --slave /usr/bin/g++ g++ /usr/bin/g++-4.7
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.6 40 --slave /usr/bin/g++ g++ /usr/bin/g++-4.6
pushd $HOME/yajl
./configure
sudo make install
popd
