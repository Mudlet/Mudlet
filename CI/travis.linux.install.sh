#!/bin/bash
sudo apt-get install build-essential qtbase5-dev qt5-default qtmultimedia5-dev qttools5-dev libhunspell-dev lua5.1 liblua5.1-0-dev libpcre3-dev libboost-dev zlib1g-dbg zlib1g-dev libzip-dev libpulse-dev cmake
cd yajl
./configure
sudo make install
cd ..

