#!/bin/sh
ipcrm -W
PIDS=`ps | grep build/test | grep -v grep | cut -f1 -d' '`
if [ ! -z "$PIDS" ] ; then
  echo cleaning "$PIDS"
  kill -9 $PIDS;
else
  echo no PIDS found.
fi
