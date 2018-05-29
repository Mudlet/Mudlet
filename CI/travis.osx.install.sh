#!/bin/bash
if [ "$TRAVIS_EVENT_TYPE" = "cron" ]; then
  echo Job not executed under cron run
  exit
fi

set +e
BREWS="boost cmake hunspell libzip libzzip lua51 pcre pkg-config qt5 yajl ccache pugixml"
for i in $BREWS; do
  for RETRIES in $(seq 1 3); do
    echo "Upgrading ${i}"
    brew outdated | grep -q $i
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
    echo "Installing ${i}"
    brew list | grep -q $i || brew install $i
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
