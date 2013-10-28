#!/bin/bash

echo $0
BASEDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "base dir $BASEDIR"
cd "$BASEDIR"
ls -l ../../python/collab/sessionjoin.py
nodepid=0

trap closeem SIGUSR2
trap closeem SIGTERM
trap closeem SIGKILL
trap closeem INT

closeem() {
    echo "closing subprocesses... jobs: $(jobs -p)"
    kill -9 $(jobs -p)
    if [ x$nodepid != x0 ]; then 
	kill -9 $nodepid
    fi
    exit
}

node server.js &
nodepid=$!
../../python/collab/sessionjoin.py > /dev/null 2>&1 &
pypid=$!

# wait

#
# Check on the children every second, restart them if need be
#
while true
do
  sleep 1
  echo "bash control script checking children..."

  if ! kill -0 $pypid >/dev/null 2>/dev/null; then
      echo "restarting python collab script..."
      ../../python/collab/sessionjoin.py > /dev/null 2>&1 &
      pypid=$!
  fi

done;

