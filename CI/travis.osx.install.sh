#!/bin/bash
set -ev
BREWS="boost cmake hunspell libzip libzzip lua pcre pkg-config qt5 yajl"
for i in $BREWS; do
  brew outdated | grep -q $i && brew upgrade $i
done
for i in $BREWS; do
  brew list | grep -q $i || brew install $i
done
