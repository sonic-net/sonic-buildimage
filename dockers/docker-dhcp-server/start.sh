#!/usr/bin/env bash

if [ "${RUNTIME_OWNER}" == "" ]; then
    RUNTIME_OWNER="kube"
fi

# This script is to basicly check for if this starting-container can be allowed
# to run based on current state, and owner & version of this starting container.
# If allowed, update feature info in state_db and then processes in supervisord.conf
# after this process can start up.
CTR_SCRIPT="/usr/share/sonic/scripts/container_startup.py"
if test -f ${CTR_SCRIPT}
then
    ${CTR_SCRIPT} -f dhcp_server -o ${RUNTIME_OWNER} -v ${IMAGE_VERSION}
fi
