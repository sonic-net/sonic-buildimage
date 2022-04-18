#!/bin/bash
set -e

source nokia-env

make init
make PLATFORM=broadcom configure
make target/sonic-broadcom.bin || ( rm -f target/*.bin && make target/sonic-broadcom.bin )
