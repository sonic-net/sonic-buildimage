#!/usr/bin/env bash
set -e

# docker-sonic-vs embeds the FRR supervisor configuration, but its STATE_DB is
# started by that same supervisor. Initialize the shared environment without
# querying restart state before supervisor is running.
source /usr/bin/frr_startup_init.sh
initialize_frr_startup_environment --normal-start

exec /usr/local/bin/supervisord "$@"
