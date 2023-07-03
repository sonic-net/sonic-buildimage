#!/usr/bin/env bash
CFGGEN_PARAMS=" \
    -d \
    -t /usr/share/sonic/templates/lldpd.conf.j2 \
    -y /etc/sonic/sonic_version.yml \
    -t /usr/share/sonic/templates/lldpdSysDescr.conf.j2 \
"

if [ "${RUNTIME_OWNER}" == "" ]; then
    RUNTIME_OWNER="kube"
fi

CTR_SCRIPT="/usr/share/sonic/scripts/container_startup.py"
if test -f ${CTR_SCRIPT}
then
    ${CTR_SCRIPT} -f lldp -o ${RUNTIME_OWNER} -v ${IMAGE_VERSION}
fi

sonic-cfggen $CFGGEN_PARAMS > /etc/lldpd.conf

mkdir -p /var/sonic
echo "# Config files managed by sonic-config-engine" > /var/sonic/config_status

rm -f /var/run/lldpd.socket
