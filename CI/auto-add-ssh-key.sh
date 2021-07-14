#!/bin/sh

# A substitute sshaskpass that merely returns the password that is stored in the
# enviromental variable - so that ssh-add avoids asking for it in a
# non-interactive case that we make it think IS interactive...
echo ${DEPLOY_KEY_PASS}
