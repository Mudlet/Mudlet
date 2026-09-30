#!/bin/bash
while systemctl --user is-active --quiet "$1"; do sleep 30; done
systemctl --user status "$1" --no-pager 2>&1 | tail -3
ls /home/vadi/.claude/jobs/3ae74264/tmp/results10
