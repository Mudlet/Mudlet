#!/bin/bash
# credit https://stackoverflow.com/a/13034313/72944

if [ $# -ne 2 ] ; then
  echo "Usage: ssh-add-pass keyfile password"
  exit 1
fi

eval "$(ssh-agent)"
pass="$2"

expect << EOF
  spawn ssh-add $1
  expect "Enter passphrase"
  send "$pass\r"
  expect eof
EOF
