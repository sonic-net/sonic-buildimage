#!/usr/bin/env bash

umount /etc/resolv.conf
ln -sf /host_etc/resolv.conf /etc/resolv.conf
