#!/bin/bash

count=10
while [ $count -gt 0 ]
do
  lsmod | grep 9641 >/dev/null 2>&1
  if [ $? -eq 0 ]
  then
    break
  fi
  count=$(( count - 1 ))
  sleep 1
done

if [ $count -eq 0 ]
then
  # mod not loaded
  exit 1
fi

if [ ! -d "/sys/bus/i2c/devices/i2c-2" ]
then
  echo pca9541 0x10 > /sys/bus/i2c/devices/i2c-0/new_device
  if [ $? -ne 0 ]
  then
    exit $?
  fi
fi

exit 0

