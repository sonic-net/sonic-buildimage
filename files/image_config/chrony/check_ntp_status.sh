#!/bin/bash

if chronyc -c tracking | grep -q "Not synchronised"; then
    echo "NTP is not synchronized with servers"
    exit 1
fi
