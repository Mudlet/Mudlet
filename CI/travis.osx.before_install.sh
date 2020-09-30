#!/bin/bash
set -ev

echo "Running brew bundle cleanup --force..."
brew bundle cleanup --force
echo "Running brew bundle install..."
brew bundle install
echo "Done with brew bundle install.
