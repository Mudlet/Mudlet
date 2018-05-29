#!/bin/bash
if [ "$TRAVIS_EVENT_TYPE" = "cron" ]; then
  echo Job not executed under cron run
  exit
fi

set +e
BREWS="boost cmake hunspell libzip libzzip lua51 pcre pkg-config qt5 yajl ccache pugixml"
for i in $BREWS; do
  for RETRIES in $(seq 1 3); do
    brew outdated | grep -q $i && brew upgrade $i
    STATUS="$?"
    if [ "${STATUS}" -eq 0 ]; then
      break
    fi
  done
done
for i in $BREWS; do
  for RETRIES in $(seq 1 3); do
    brew list | grep -q $i || brew install $i
    STATUS="$?"
    if [ "${STATUS}" -eq 0 ]; then
      break
    fi
  done
done
