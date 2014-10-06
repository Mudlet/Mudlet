#!/bin/bash
set -ev
sudo apt-add-repository ppa:ubuntu-sdk-team/ppa -y
sudo apt-add-repository ppa:kalakris/cmake -y
# newer GCC version, 4.7
sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
sudo apt-get update
sudo apt-get install gcc-4.7 g++-4.7
sudo update-alternatives --remove gcc /usr/bin/gcc-4.6
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.7 60 --slave /usr/bin/g++ g++ /usr/bin/g++-4.7
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.6 40 --slave /usr/bin/g++ g++ /usr/bin/g++-4.6
pushd $HOME
git clone https://github.com/lloyd/yajl.git
popd
