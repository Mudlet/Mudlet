#!/bin/bash

set +e
shopt -s expand_aliases
#Removed boost as first item as a temporary workaroud to prevent trying to
#upgrade to boost version 1.68.0 which has not been bottled yet...
BREWS="cmake hunspell libzip libzzip lua51 pcre pkg-config qt5 yajl ccache pugixml luarocks"
OUTDATED_BREWS=$(brew outdated)

for i in $BREWS; do
  for RETRIES in $(seq 1 3); do
    echo " "
    echo "Considering whether to upgrade: ${i}"
    #Added the -w (whole-word) option so that the grep will NOT match for pcre2
    #when we are considering pcre:
    echo "${OUTDATED_BREWS}" | grep -w -q $i
    STATUS="$?"
    if [ "${STATUS}" -ne 0 ]; then
      echo "Already up to date or not installed."
      break
    fi

    brew upgrade $i
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
  ZIPLIBDIR=$(pkg-config --variable=libdir zziplib)
  ZIPLIBVERSION=$(pkg-config --modversion zziplib)
  if [ "${ZIPLIBVERSION}" == "0.13.71" ]; then
    echo "pkg-config says that zziplib is installed and is version 0.13.71."
    echo "library symbolic links:"
    echo "Compile flags are: $(pkg-config --cflags zziplib) "
    echo "Header directory is: $(pkg-config --variable=includedir zziplib) "
    echo "Link flags are: $(pkg-config --libs zziplib) "
    echo "Library directory is: ${ZIPLIBDIR} "
    echo "and that directory before modification contained:"
    ls -l ${ZIPLIBDIR}
    sudo ln -s ${ZIPLIBDIR}/libzzip-${ZIPLIBVERSION}.dylib ${ZIPLIBDIR}/libzzip.dylib
    sudo ln -s ${ZIPLIBDIR}/libzzipfseeko-${ZIPLIBVERSION}.dylib ${ZIPLIBDIR}/libzzipfseeko.dylib
    sudo ln -s ${ZIPLIBDIR}/libzzipmmapped-${ZIPLIBVERSION}.dylib ${ZIPLIBDIR}/libzzipmmapped.dylib
    sudo ln -s ${ZIPLIBDIR}/libzzipwrap-${ZIPLIBVERSION}.dylib ${ZIPLIBDIR}/libzzipwrap.dylib
  else
    echo "pkg-config says that zziplib is installed and is version ${ZIPLIBVERSION}, so NOT "
    echo "bodging some library symbolic links for version 0.13.71 .  Indeed it may be "
    echo "time to delete a section from the ./CI/travis.osx.install.sh file!"
  fi
  echo "The library directory for zziplib now contains:"
  ls -l ${ZIPLIBDIR}
fi
echo "*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*="
