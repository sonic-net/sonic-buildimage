#!/usr/bin/env bash

if [ "${RUNTIME_OWNER}" == "" ]; then
    RUNTIME_OWNER="kube"
fi

CTR_SCRIPT="/usr/share/sonic/scripts/container_startup.py"
if test -f ${CTR_SCRIPT}
then
    ${CTR_SCRIPT} -f otel -o ${RUNTIME_OWNER} -v ${IMAGE_VERSION}
fi

mkdir -p /var/sonic
echo "# Config files managed by sonic-config-engine" > /var/sonic/config_status
