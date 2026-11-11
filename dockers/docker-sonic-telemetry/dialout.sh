#!/usr/bin/env bash

# Start with default config
export CVL_SCHEMA_PATH=/usr/sbin/schema
exec /usr/sbin/dialout_client_cli -logtostderr  -v 2
