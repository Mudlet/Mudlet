#!/bin/bash

mkdir -p "${HOME}/latest-gcc-symlinks"
ln -s /usr/bin/g++-7 "${HOME}/latest-gcc-symlinks/g++"
ln -s /usr/bin/gcc-7 "${HOME}/latest-gcc-symlinks/gcc"

# lua-utf8 is not in the repositories...
luarocks install --local luautf8
YAJL_PATH="$(pkg-config --variable=libdir yajl)"
echo "Setting search path to $YAJL_PATH"
luarocks install --local lua-yajl YAJL_LIBDIR="${YAJL_PATH}"

# CI changelog generation dependencies
luarocks install --local argparse
luarocks install --local lunajson

  # download coverity tool only for cron+deploy jobs
if [ "${TRAVIS_EVENT_TYPE}" = "cron" ] && [ "${DEPLOY}" = "deploy" ]; then
  mkdir coverity
  cd coverity
  wget https://scan.coverity.com/download/linux64 --post-data "token=${COVERITY_SCAN_TOKEN}&project=Mudlet%2FMudlet" -O coverity_tool.tgz
  if [ $? -ne 0 ]; then
	  echo Download Coverity analysis tool failed!
	  exit 1
  fi
  tar xzf coverity_tool.tgz
  rm -f coverity_tool.tgz
  export PATH="$(pwd)/$(ls -d cov*)/bin:$PATH"
  echo The new PATH is now "${PATH}"
  cd ..
# Coverity scan tool installed
fi
