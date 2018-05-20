#!/bin/bash
compile_line=()
if [ "${TRAVIS_EVENT_TYPE}" = "cron" ]; then
  if [ "${CC}" = "clang" ] || [ "${Q_OR_C_MAKE}" = "cmake" ] || [ "${QT_VERSION}" = "56" ]; then
    echo Job not executed under cron run
    exit 0
  fi
  export CCACHE_DISABLE=1
  compile_line+=(cov-build --dir cov-int)
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
  qmake ${SPEC} ../src/mudlet.pro
else
  cmake --version
  cmake ..
fi

compile_line+=(make -j ${PROCS})

set -x
"${compile_line[@]}"
