#!/bin/bash
compile_line=()
# macOS PTB is cron+deploy
# linux coverity is cron+deploy
# linux PTB is cron+clang+cmake
if { [ "${TRAVIS_EVENT_TYPE}" = "cron" ] && [ "${TRAVIS_OS_NAME}" = "osx" ] && [ "${DEPLOY}" = "deploy" ]; } ||
   { [ "${TRAVIS_EVENT_TYPE}" = "cron" ] && [ "${TRAVIS_OS_NAME}" = "linux" ] && [ "${DEPLOY}" = "deploy" ]; } ||
   { [ "${TRAVIS_EVENT_TYPE}" = "cron" ] && [ "${TRAVIS_OS_NAME}" = "linux" ] &&  [ "${CC}" = "clang" ] && [ "${Q_OR_C_MAKE}" = "cmake" ]; } then
  export CCACHE_DISABLE=1

  if [ "${TRAVIS_EVENT_TYPE}" = "cron" ] && [ "${TRAVIS_OS_NAME}" = "linux" ] && [ "${DEPLOY}" = "deploy" ]; then
    compile_line+=(cov-build --dir cov-int)
  fi
elif [ "${TRAVIS_EVENT_TYPE}" = "cron" ]; then
  echo Job not executed under cron run
  exit 0
fi

cd build || exit
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
