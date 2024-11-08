#!/usr/bin/env bash


if [ "${RUNTIME_OWNER}" == "" ]; then
    RUNTIME_OWNER="kube"
fi

CTR_SCRIPT="/usr/share/sonic/scripts/container_startup.py"
if test -f ${CTR_SCRIPT}
then
    ${CTR_SCRIPT} -f bmp -o ${RUNTIME_OWNER} -v ${IMAGE_VERSION}
fi

mkdir -p /etc/bmp

is_multi_asic=$(python3 -c "from sonic_py_common.multi_asic import is_multi_asic; print(is_multi_asic())")

if [[ $is_multi_asic == "True" ]]; then
    export multiASIC=true
else
    export multiASIC=false
fi

ENABLE_MULTI_ASIC=$multiASIC j2 /usr/share/sonic/templates/openbmpd.conf.j2 > /etc/bmp/openbmpd.conf


