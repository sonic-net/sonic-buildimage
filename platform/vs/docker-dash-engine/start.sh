#!/usr/bin/env bash

PORT1=eth1
PORT2=eth2

[[ $(cat /sys/class/net/$PORT1/operstate) != up ]] && ifconfig $PORT1 up
[[ $(cat /sys/class/net/$PORT2/operstate) != up ]] && ifconfig $PORT2 up

simple_switch_grpc --interface 0@$PORT1 --interface 1@$PORT2 --log-console --no-p4
