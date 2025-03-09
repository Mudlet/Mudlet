#!/bin/bash

set +e
shopt -s expand_aliases
#Removed boost as first item as a temporary workaround to prevent trying to
#upgrade to boost version 1.68.0 which has not been bottled yet...
BREWS="luarocks cmake hunspell libzip mudlet/dependencies/lua@5.1 pcre pkg-config yajl ccache pugixml qt6"
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
    brew install "$i"
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
gem update cocoapods

# create an alias to avoid the need to list the lua dir all the time
# we want to expand the subshell only once (it's only temporary anyways)
# shellcheck disable=2139
alias luarocks-5.1="luarocks --lua-dir='$(brew --prefix mudlet/dependencies/lua@5.1)'"
LIBRARY_PATH=/opt/homebrew/Cellar/yajl/2.1.0/lib C_INCLUDE_PATH=/opt/homebrew/Cellar/yajl/2.1.0/include luarocks-5.1 --local install lua-yajl
