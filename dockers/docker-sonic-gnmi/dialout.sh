#!/usr/bin/env bash

# Start with default config
export CVL_SCHEMA_PATH=/usr/sbin/schema
exec /usr/sbin/dialout_client_cli -insecure -logtostderr  -v 2

