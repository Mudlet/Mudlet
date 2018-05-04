#!/bin/bash
if [ "${COVERITY_SCAN_BRANCH}" = 1 ]; then
  echo "Already built and uploaded to coverity. Skipping build..."
  exit 0
fi

cd build
if [ "${TRAVIS_OS_NAME}" = "osx" ]; then
  # need to set additional compile flags on osx
  export LDFLAGS=" -L/usr/local/opt/qt5/lib ${LDFLAGS}"
  export CPPFLAGS=" -I/usr/local/opt/qt5/include ${CPPFLAGS}"
fi
if [ "${CC}" = "clang" ] && [ "${TRAVIS_OS_NAME}" = "linux" ] && [ "${Q_OR_C_MAKE}" = "qmake" ] ; then
  # need to specify the clang specs for linux qmake builds
  SPEC="-spec linux-clang"
fi
if [ "${TRAVIS_OS_NAME}" = "osx" ]; then # getting the number of processors is different for each OS
  PROCS=$(sysctl -n hw.physicalcpu)
else
  PROCS=$(nproc)
fi
echo "Compiling using ${PROCS} cores."
if [ "${Q_OR_C_MAKE}" = "qmake" ]; then
  qmake -v
  qmake ${SPEC} ../src/mudlet.pro && make -j ${PROCS}
else
  cmake --version
  cmake .. && make -j ${PROCS} && make check
fi
