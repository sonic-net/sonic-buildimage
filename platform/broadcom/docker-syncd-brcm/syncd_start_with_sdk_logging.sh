#!/usr/bin/env bash
# Use the upstream startup configuration, then instrument its final -p profile.
. /usr/bin/syncd_init_common.sh
config_syncd
exec /usr/bin/broadcom_sdk_logging.py ${CMD} ${CMD_ARGS}
