#!/bin/bash
# This script needs to be run as root (http://blog.inventic.eu/2012/08/how-to-deploy-qt-application-on-macos-part-ii/)
# so if it's asking for a password, do sudo chmod +s mac-deploy.sh or enter
# the password everytime
#
# This script needs to be run from inside the src/ folder.

echo Copying Qt and dependent libraries...
export PATH=$PATH:$HOME/Qt/5.1.1/clang_64/bin
echo Using: $PATH to find macdeployqt

# Do not use ./mudlet.app here, but just muldlet.app - OSX will mount is wrong otherwise
# don't generate the .dmg, just the app - node-appdmg handles nice icons and the dmg
sudo macdeployqt Mudlet.app
echo Done!
