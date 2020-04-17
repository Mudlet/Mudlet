#!/bin/bash

set +e
shopt -s expand_aliases
#Removed boost as first item as a temporary workaroud to prevent trying to
#upgrade to boost version 1.68.0 which has not been bottled yet...
BREWS="cmake hunspell libzip libzzip lua51 pcre pkg-config qt5 yajl ccache pugixml luarocks"
OUTDATED_BREWS=$(brew outdated)
echo "The following brews are outdated:"
for b in ${OUTDATED_BREWS}; do
    echo "    ${b}"
done
echo "End of outdated brew listing."

for i in ${BREWS}; do
  for RETRIES in $(seq 1 3); do
    echo " "
    echo "Considering whether to upgrade: ${i}"
    #Added the -w (whole-word) option so that the grep will NOT match for pcre2
    #when we are considering pcre:
    #Removed the -q option to grep for debugging
    echo "${OUTDATED_BREWS}" | grep -w ${i}
    STATUS="$?"
    if [ "${STATUS}" -eq 1 ]; then
      echo "Already up to date or not installed."
      break
    elif [ "${STATUS}" -eq 2 ]; then
      echo "Error!"
      exit 1
    else
      echo "Needs updating..."
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
