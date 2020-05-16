#!/bin/bash
compile_line=()
if [ "${TRAVIS_EVENT_TYPE}" = "cron" ]; then
  if [ "${DEPLOY}" != "deploy" ]; then
    echo Job not executed under cron run
    exit 0
  fi
  export CCACHE_DISABLE=1

  if [ "${TRAVIS_OS_NAME}" = "linux" ]; then
    compile_line+=(cov-build --dir cov-int)
  fi
fi

cd build
if [ "${TRAVIS_OS_NAME}" = "osx" ]; then
  # need to set additional compile flags on osx
  export LDFLAGS=" -L/usr/local/opt/qt5/lib ${LDFLAGS}"
  export CPPFLAGS=" -I/usr/local/opt/qt5/include ${CPPFLAGS}"
fi
if [ "${TRAVIS_OS_NAME}" = "osx" ]; then # getting the number of processors is different for each OS
  PROCS=$(sysctl -n hw.physicalcpu)
else
  PROCS=$(nproc)
fi
"${Q_OR_C_MAKE}" --version
echo "Compiling using ${PROCS} cores."
if [ "${Q_OR_C_MAKE}" = "qmake" ]; then
  qmake "${SPEC}" ../src/mudlet.pro
else
  cmake ..
fi

compile_line+=(make -j "${PROCS}")

set -x
"${compile_line[@]}"
