#!/bin/bash

set +e
shopt -s expand_aliases
#Removed boost as first item as a temporary workaroud to prevent trying to
#upgrade to boost version 1.68.0 which has not been bottled yet...
BREWS="cmake hunspell libzip libzzip lua51 pcre pkg-config qt5 yajl ccache pugixml luarocks"
OUTDATED_BREWS=$(brew outdated)

for i in ${BREWS}; do
  for RETRIES in $(seq 1 3); do
    echo " "
    echo "Considering whether to upgrade: ${i}"
    #Added the -w (whole-word) option so that the grep will NOT match for pcre2
    #when we are considering pcre:
    echo "${OUTDATED_BREWS}" | grep -w -q ${i}
    STATUS="$?"
    if [ "${STATUS}" -ne 0 ]; then
      echo "Already up to date or not installed."
      break
    fi

    brew upgrade ${i}
    STATUS="$?"
    if [ "${STATUS}" -eq 0 ]; then
      break
    fi
    echo "Attempt ${RETRIES} failed."
    if [ "${RETRIES}" -eq 3 ]; then
      echo "Too many retries. Aborting."
      exit 1
    else
      echo "Retrying..."
    fi
  done
done
for i in $BREWS; do
  for RETRIES in $(seq 1 3); do
    echo " "
    echo "Installing ${i}"
    #Added the -w (whole-word) option so that the grep will NOT match for pcre2
    #when we are considering pcre:
    brew list | grep -w -q $i || brew install $i
    STATUS="$?"
    if [ "${STATUS}" -eq 0 ]; then
      break
    fi
    echo "Attempt ${RETRIES} failed."
    if [ "${RETRIES}" -eq 3 ]; then
      echo "Too many retries. Aborting."
      exit 1
    else
      echo "Retrying..."
    fi
  done
done
gem install concurrent-ruby
gem update cocoapods

# create an alias to avoid the need to list the lua dir all the time
# we want to expand the subshell only once (it's only temporary anyways)
# shellcheck disable=2139
alias luarocks-5.1="luarocks --lua-dir='$(brew --prefix lua@5.1)'"
luarocks-5.1 --local install lua-yajl


echo "=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*"
echo "Some temporary debug information about libzzip / zzip / zziplib ..."
pkg-config --exists zziplib
status=$?
if [ $status -ne 0 ]; then
  echo "pkg-config does not think that zziplib is installed...!"
else
  echo "pkg-config says that zziplib is installed:"
  echo "compile flags are:"
  pkg-config --cflags zziplib
  echo "header directory is:"
  pkg-config --variable=includedir zziplib
  echo "link flags are:"
  pkg-config --libs zziplib
  echo "library directory is:"
  pkg-config --variable=libdir zziplib
  echo "That directory currently contains:"
  ls -l $(pkg-config --variable=libdir zziplib)
  echo "Bodging links from the unsuffixed named libzzip libraies to the actual files:
  ln -s $(pkg-config --variable=libdir zziplib)/libzzip-0.13.0.71.dylib $(pkg-config --variable=libdir zziplib)/libzzip.dylib
  ln -s $(pkg-config --variable=libdir zziplib)/libzzipfseeko-0.13.0.71.dylib $(pkg-config --variable=libdir zziplib)/libzzipfseeko.dylib
  ln -s $(pkg-config --variable=libdir zziplib)/libzzipmmapped-0.13.0.71.dylib $(pkg-config --variable=libdir zziplib)/libzzipmmapped.dylib
  ln -s $(pkg-config --variable=libdir zziplib)/libzzipwrap-0.13.0.71.dylib $(pkg-config --variable=libdir zziplib)/libzzipwrap.dylib
  echo "It now contains:"
  ls -l $(pkg-config --variable=libdir zziplib)
fi
echo "*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*="
