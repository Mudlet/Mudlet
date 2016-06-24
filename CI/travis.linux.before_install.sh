#!/bin/bash
set -ev
sudo apt-add-repository ppa:ubuntu-sdk-team/ppa -y
# The ubuntu team have relocated the old qt 5.0.2 (and other things) from the above to:
sudo apt-add-repository ppa:canonical-qt5-edgers/ubuntu1204-qt5 -y
sudo apt-add-repository ppa:kalakris/cmake -y
# newer GCC version, 4.7
sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
sudo add-apt-repository ppa:boost-latest/ppa -y
sudo apt-get update
pushd $HOME
git clone https://github.com/lloyd/yajl.git
popd
