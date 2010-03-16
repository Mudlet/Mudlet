#!/bin/bash
cores=`grep processor /proc/cpuinfo | wc -l`
echo "$cores coress found using them to build Mudlet\n"
J=$((cores))
echo "J=$J"
if [ -d src/build ]; then
     cd src/build
     make clean
else
     mkdir src/build
     cd src/build
fi
cmake ../ -DCMAKE_INSTALL_REFIX=. && make -j$J && make package
