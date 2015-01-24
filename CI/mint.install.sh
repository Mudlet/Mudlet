
set -ev
sudo apt-get update
sudo apt-get install freeglut3
sudo apt-get install freeglut3-dev
sudo apt-get install binutils-gold
sudo apt-get install g++ cmake
sudo apt-get install libglew-dev
sudo apt-get install mesa-common-dev
sudo apt-get install build-essential
sudo apt-get install libglew1.5-dev libglm-dev


sudo apt-get install \
  build-essential \
  qt5-default qtmultimedia5-dev qttools5-dev \
  libqt5opengl5-dev \
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

pushd /tmp 
git clone https://github.com/lloyd/yajl.git
pushd /tmp/yajl
./configure
sudo make install
popd
popd

