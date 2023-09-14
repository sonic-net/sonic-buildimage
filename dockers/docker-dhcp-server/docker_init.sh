#!/usr/bin/env bash

# The docker container should start this script as PID 1, so now that supervisord is
# properly configured, we exec /usr/local/bin/supervisord so that it runs as PID 1 for the
# duration of the container's lifetime
exec /usr/local/bin/supervisord
