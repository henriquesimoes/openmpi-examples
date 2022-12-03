#!/bin/bash

# Assert the script continues only on success
set -e;

hostnames=`cat /hosts/names | cut -d' ' -f 1`
cp /hosts/config $SSH_PATH/config

service ssh restart

for name in $hostnames; do
  echo "Testing communication with machine $name..."
  ssh $name echo "Working just fine."
done
