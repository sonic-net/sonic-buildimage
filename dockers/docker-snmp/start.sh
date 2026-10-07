#!/usr/bin/env bash


if [ "${RUNTIME_OWNER}" == "" ]; then
    RUNTIME_OWNER="kube"
fi

CTR_SCRIPT="/usr/share/sonic/scripts/container_startup.py"
# IMAGE_VERSION is exported by the SONiC host when the feature container is
# launched. In the monolithic docker-sonic-vs image the snmp start.sh runs as
# an in-image supervisor program where IMAGE_VERSION is not set, and
# container_startup.py aborts on the empty -v argument. Skip the kube/local
# container bookkeeping when it (or the script) is unavailable.
if test -f ${CTR_SCRIPT} && [ -n "${IMAGE_VERSION}" ]
then
    ${CTR_SCRIPT} -f snmp -o ${RUNTIME_OWNER} -v ${IMAGE_VERSION}
fi

mkdir -p /etc/ssw /etc/snmp

# In the standalone docker-snmp container, IMAGE_VERSION is set and the
# existing startup contract imports /etc/sonic/snmp.yml into CONFIG_DB. In the
# monolithic docker-sonic-vs image, IMAGE_VERSION is unset and cSONiC loads its
# complete config_db.json after the image-wide start.sh exits. Wait there until
# both DEVICE_METADATA and a configured SNMP community are present. This avoids
# rendering from an incomplete DB without baking a public fallback community
# into the image.
SNMP_CONFIG_WAIT_SECS="${SNMP_CONFIG_WAIT_SECS:-60}"
snmp_config_ready=yes

if [ -n "${IMAGE_VERSION}" ]; then
    /usr/bin/snmp_yml_to_configdb.py
else
    snmp_config_ready=no
    for _i in $(seq 1 "${SNMP_CONFIG_WAIT_SECS}"); do
        if [ -n "$(sonic-db-cli CONFIG_DB HGET 'DEVICE_METADATA|localhost' 'hwsku' 2>/dev/null)" ] \
           && [ -n "$(sonic-db-cli CONFIG_DB KEYS 'SNMP_COMMUNITY|*' 2>/dev/null)" ]; then
            snmp_config_ready=yes
            logger -t start-snmp -p daemon.info \
                "DEVICE_METADATA and SNMP_COMMUNITY present in CONFIG_DB after ${_i}s"
            break
        fi
        sleep 1
    done
fi

SONIC_CFGGEN_ARGS=" \
    -d \
    -y /etc/sonic/sonic_version.yml \
    -t /usr/share/sonic/templates/sysDescription.j2,/etc/ssw/sysDescription \
    -t /usr/share/sonic/templates/snmpd.conf.j2,/etc/snmp/snmpd.conf \
"

sonic-cfggen $SONIC_CFGGEN_ARGS

mkdir -p /var/sonic
echo "# Config files managed by sonic-config-engine" > /var/sonic/config_status

# Render first, then fail: snmpd.conf is rewritten from CONFIG_DB on every run, so
# a timeout leaves a config with no rocommunity/rwcommunity line. That is fail-closed
# (snmpd answers nothing) and deliberately replaces the stock Debian snmpd.conf, which
# would otherwise still be in place and could serve a default community. What must not
# happen is doing that silently, so report the timeout and exit non-zero.
if [ "${snmp_config_ready}" != "yes" ]; then
    logger -t start-snmp -p daemon.error \
        "No SNMP_COMMUNITY in CONFIG_DB after ${SNMP_CONFIG_WAIT_SECS}s; wrote /etc/snmp/snmpd.conf with no community. snmpd will not answer any query until SNMP_COMMUNITY is configured and this script is re-run."
    echo 1>&2 "error: start-snmp: SNMP_COMMUNITY not found in CONFIG_DB after ${SNMP_CONFIG_WAIT_SECS}s"
    exit 1
fi
