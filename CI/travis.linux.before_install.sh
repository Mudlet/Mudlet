#!/bin/bash
set -ev
sudo apt-add-repository ppa:ubuntu-sdk-team/ppa -y
sudo apt-add-repository ppa:kalakris/cmake -y
sudo apt-get update
pushd $HOME
git clone https://github.com/lloyd/yajl.git
popd
