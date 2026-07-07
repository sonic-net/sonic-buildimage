#!/bin/bash

set -euo pipefail

NETWORK_TIME=$({ chronyc -n -c tracking | cut -d, -f 4 | cut -d. -f 1; } || echo 0)
if [[ ${NETWORK_TIME} -gt 0 ]]; then
	touch -t $(date -d @${NETWORK_TIME} +%Y%m%d%H%M.%S) -m /host/clock-epoch
else
	touch -m /host/clock-epoch
fi
