#!/usr/bin/bash
# This script overwrites reboot to an empty function before running the given command.

reboot () {
    echo "reboot is disabled"
}

export -f reboot
$@
