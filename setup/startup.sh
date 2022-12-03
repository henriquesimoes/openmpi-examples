#!/bin/bash

hostname=`cat /etc/hostname`
hostfile=/hosts/names

echo "$hostname slots=2 max_slots=2" >> $hostfile
cat << EOF >> /hosts/config
Host $hostname $hostname
  HostName $hostname
  IdentityFile $SSH_KEY
  PubKeyAuthentication yes
  StrictHostKeyChecking no

EOF

if [[ $# -gt 1 ]] && [[ $1 = "master" ]]; then
  cat $hostfile | sort | uniq > $hostfile
fi

# start SSH service
service ssh start
