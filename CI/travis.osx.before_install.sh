#!/bin/bash
set -ev
brew update-reset

# Ensure Homebrew's npm is used, instead of an outdated one
PATH=/usr/local/bin:$PATH
echo "Running npm install. PATH is $PATH"
npm install -g appdmg
echo "Successfully ran npm install."

echo "Making .npm-global..."
mkdir /Users/travis/.npm-global
echo "1 Checking /Users/travis/.npm-global..."
file /Users/travis/.npm-global
