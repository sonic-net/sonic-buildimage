#!/usr/bin/python
import fcntl, struct
import time
from os import path

RNDADDENTROPY=0x40085203

def avail():
  with open("/proc/sys/kernel/random/entropy_avail", mode='r') as avail:
      return int(avail.read())

if path.exists("/proc/sys/kernel/random/entropy_avail"):
    while 1:
        while avail() < 2048:
            with open('/dev/urandom', 'rb') as urnd, open("/dev/random", mode='wb') as rnd:
                d = urnd.read(512)
                t = struct.pack('ii', 4 * len(d), len(d)) + d
                fcntl.ioctl(rnd, RNDADDENTROPY, t)
        time.sleep(30)
