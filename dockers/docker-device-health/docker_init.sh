#!/usr/bin/env bash

cp /usr/share/lom/LoM-Version.json /usr/share/device_health/

# Generate supervisord config file
mkdir -p /etc/supervisor/conf.d/

# Generate supervisord configuration template.
TEMPLATE_FILE="/usr/share/sonic/templates/lom.supervisord.conf.j2"
PROCS_JSON_FILE="/usr/share/lom/procs.conf.json"
SUPERVISORD_FILE="/etc/supervisor/conf.d/LoMSupervisord.conf"

j2 -f json -o ${SUPERVISORD_FILE} ${TEMPLATE_FILE} ${PROCS_JSON_FILE}

# The docker container should start this script as PID 1, so now that supervisord is
# properly configured, we exec /usr/local/bin/supervisord so that it runs as PID 1 for the
# duration of the container's lifetime
exec /usr/local/bin/supervisord
