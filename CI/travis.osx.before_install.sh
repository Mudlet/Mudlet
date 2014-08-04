#!/bin/bash
brew update
for i in <packages to check>; do
  brew outdated | grep -q $i && brew upgrade $i
done
git clone https://github.com/lloyd/yajl.git

