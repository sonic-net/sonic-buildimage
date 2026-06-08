#!/bin/bash

sudo /usr/sbin/groupadd -f dialout

awk -F: '$3 >= 1000 && $1 != "nobody" {print $1}' /etc/passwd | while read user; do
    sudo /usr/sbin/usermod -aG dialout "$user"
done
