#!/bin/bash
set -e

source nokia-env

make init
make PLATFORM=broadcom configure
make SONIC_BUILD_JOBS=2 ENABLE_SYNCD_RPC=y \
  target/docker-syncd-brcm-dnx-rpc.gz
