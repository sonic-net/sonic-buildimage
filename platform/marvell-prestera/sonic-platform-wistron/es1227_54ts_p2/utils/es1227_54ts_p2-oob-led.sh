#!/bin/bash

init()
{
    # init oob port led
    phytool write eth0/0/22 3
    phytool write eth0/0/17 0x4400
    phytool write eth0/0/18 0x0080
    phytool write eth0/0/16 0x0340
    phytool write eth0/0/22 0
}

main()
{
    init
}

main
