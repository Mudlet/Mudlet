#!/bin/bash
if [ "$TRAVIS_EVENT_TYPE" = "cron" ]; then
	echo Job not executed under cron run
	exit
fi

set -e
BREWS="boost cmake hunspell libzip libzzip lua51 pcre pkg-config qt5 yajl"
for i in $BREWS; do
  brew outdated | grep -q $i && brew upgrade $i
done
for i in $BREWS; do
  brew list | grep -q $i || brew install $i
done
