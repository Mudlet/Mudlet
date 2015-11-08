#!/bin/bash
set -ev
echo Running travis.osx.install.sh...
BREWS="boost cmake hunspell libzip libzzip lua51 pcre pkg-config qt5 yajl"
for i in $BREWS; do
  brew outdated | grep -q $i && brew upgrade $i
done
for i in $BREWS; do
  brew list | grep -q $i || brew install $i
done
brew linkapps qt5
