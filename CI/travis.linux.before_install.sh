#!/bin/bash
set -ev
sudo apt-add-repository ppa:ubuntu-sdk-team/ppa -y
sudo apt-add-repository ppa:kalakris/cmake -y
# newer GCC version, 4.7
sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
sudo add-apt-repository ppa:boost-latest/ppa -y
sudo apt-get update
pushd $HOME
git clone https://github.com/lloyd/yajl.git
popd
