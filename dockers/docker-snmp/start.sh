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

# snmp_yml_to_configdb.py and `sonic-cfggen -d` both read CONFIG_DB. In the
# standalone docker-snmp container CONFIG_DB is already populated by the host
# before the container starts, but in the monolithic docker-sonic-vs image this
# start.sh can run before the image-wide start.sh has finished loading
# CONFIG_DB, which renders snmpd.conf from an empty DB (snmpd then binds only to
# loopback with the stock Debian community and is unreachable). Wait for
# CONFIG_DB to be initialized (DEVICE_METADATA present) before rendering.
for _i in $(seq 1 60); do
    if sonic-db-cli CONFIG_DB HGET 'DEVICE_METADATA|localhost' 'hwsku' >/dev/null 2>&1 \
       && [ -n "$(sonic-db-cli CONFIG_DB HGET 'DEVICE_METADATA|localhost' 'hwsku' 2>/dev/null)" ]; then
        break
    fi
    sleep 1
done

# Parse snmp.yml and insert the data in Config DB
/usr/bin/snmp_yml_to_configdb.py

SONIC_CFGGEN_ARGS=" \
    -d \
    -y /etc/sonic/sonic_version.yml \
    -t /usr/share/sonic/templates/sysDescription.j2,/etc/ssw/sysDescription \
    -t /usr/share/sonic/templates/snmpd.conf.j2,/etc/snmp/snmpd.conf \
"

sonic-cfggen $SONIC_CFGGEN_ARGS

mkdir -p /var/sonic
echo "# Config files managed by sonic-config-engine" > /var/sonic/config_status
