#!/bin/bash
set -ev
brew update-reset

# Ensure Homebrew's npm is used, instead of an outdated one
echo "Running npm install. PATH is $PATH"
PATH=/usr/local/bin:$PATH
npm install -g appdmg
echo "Successfully ran npm install."

"1 Checking /Users/travis/.npm-global..."
mkdir ~/.npm-global
file /Users/travis/.npm-global
