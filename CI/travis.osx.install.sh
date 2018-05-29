#!/bin/bash
if [ "$TRAVIS_EVENT_TYPE" = "cron" ]; then
  echo Job not executed under cron run
  exit
fi

set +e
BREWS="boost cmake hunspell libzip libzzip lua51 pcre pkg-config qt5 yajl ccache pugixml"
for i in $BREWS; do
  RETRIES=0
  while [ ${RETRIES} -lt 3 ]; do
    brew outdated | grep -q $i && brew upgrade $i
    STATUS=$?
    if [ STATUS -ne 0 ]; then
      RETRIES=$[${RETRIES}+1]
    else
      break
    fi
  done
done
for i in $BREWS; do
  RETRIES=0
  while [ ${RETRIES} -lt 3 ]; do
    brew list | grep -q $i || brew install $i
    STATUS=$?
    if [ STATUS -ne 0 ]; then
      RETRIES=$[${RETRIES}+1]
    else
      break
    fi
  done
done
