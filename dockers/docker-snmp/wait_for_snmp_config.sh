#!/bin/bash
# Wait for an SNMP community to appear in CONFIG_DB before snmpd.conf is rendered.
#
# In the standalone docker-snmp container IMAGE_VERSION is set and the existing
# startup contract imports /etc/sonic/snmp.yml into CONFIG_DB, so there is
# nothing to wait for and this exits immediately. In the monolithic
# docker-sonic-vs image IMAGE_VERSION is unset and cSONiC loads its complete
# config_db.json after the image-wide start.sh exits, so wait for both
# DEVICE_METADATA and a configured community rather than rendering from an
# incomplete DB or baking a public fallback community into the image.
#
# Safe to run more than once: it only reads CONFIG_DB, and the dependent-startup
# plugin may launch a startsecs=0 program again.

TIMEOUT="${SNMP_CONFIG_WAIT_SECS:-60}"

if [ -n "${IMAGE_VERSION}" ]; then
    exit 0
fi

for i in $(seq 1 "$TIMEOUT"); do
    if [ -n "$(sonic-db-cli CONFIG_DB HGET 'DEVICE_METADATA|localhost' 'hwsku' 2>/dev/null)" ] \
       && [ -n "$(sonic-db-cli CONFIG_DB KEYS 'SNMP_COMMUNITY|*' 2>/dev/null)" ]; then
        logger -t wait-for-snmp-config -p daemon.info \
            "DEVICE_METADATA and SNMP_COMMUNITY present in CONFIG_DB after ${i}s"
        exit 0
    fi
    sleep 1
done

logger -t wait-for-snmp-config -p daemon.error \
    "timed out waiting for SNMP_COMMUNITY in CONFIG_DB after ${TIMEOUT}s; snmpd.conf will be rendered with no community and snmpd will not answer any query until SNMP_COMMUNITY is configured"
exit 1
