#!/usr/bin/env bash

EXIT_P4RT_VARS_FILE_NOT_FOUND=1
readonly P4RT_VARS_FILE=/usr/share/sonic/templates/p4rt_vars.j2

if [ ! -f "${P4RT_VARS_FILE}" ]; then
    echo "P4rt vars template file not found"
    exit ${EXIT_P4RT_VARS_FILE_NOT_FOUND}
fi

# Try to read p4rt and certs config from ConfigDB.
# Use default value if no valid config exists
P4RT_VARS=$(sonic-cfggen -d -t ${P4RT_VARS_FILE})
readonly P4RT_VARS=${P4RT_VARS//[\']/\"}
readonly X509=$(echo ${P4RT_VARS} | jq -r '.x509')
readonly P4RT=$(echo ${P4RT_VARS} | jq -r '.p4rt')
readonly CERTS=$(echo ${P4RT_VARS} | jq -r '.certs')

P4RT_ARGS=" --alsologtostderr --logbuflevel=-1"

if [ -n "${CERTS}" ]; then
    readonly SERVER_CRT=$(echo ${CERTS} | jq -r '.server_crt // empty')
    readonly SERVER_KEY=$(echo ${CERTS} | jq -r '.server_key // empty')
    if [ -z "${SERVER_CRT}"  ] || [ -z "${SERVER_KEY}"  ]; then
        P4RT_ARGS+=" --use_insecure_server_credentials"
    else
        P4RT_ARGS+=" --server_certificate_file=${SERVER_CRT} --server_key_file=${SERVER_KEY}"
    fi

    readonly CA_CRT=$(echo ${CERTS} | jq -r '.ca_crt // empty')
    if [ ! -z "${CA_CRT}" ]; then
        P4RT_ARGS+=" --ca_certificate_file=${CA_CRT}"
        readonly CRL=$(echo ${CERTS} | jq -r '.cert_crl_dir // empty')
        if [ ! -z "$CRL" ]; then
            P4RT_ARGS+=" --cert_crl_dir=${CRL}"
        fi
    fi
elif [ -n "${X509}" ]; then
    readonly SERVER_CRT=$(echo ${X509} | jq -r '.server_crt // empty')
    readonly SERVER_KEY=$(echo ${X509} | jq -r '.server_key // empty')
    if [ -z "${SERVER_CRT}"  ] || [ -z "${SERVER_KEY}"  ]; then
        P4RT_ARGS+=" --use_insecure_server_credentials"
    else
        P4RT_ARGS+=" --server_certificate_file=${SERVER_CRT} --server_key_file=${SERVER_KEY}"
    fi

    readonly CA_CRT=$(echo ${X509} | jq -r '.ca_crt // empty')
    if [ ! -z "${CA_CRT}" ]; then
        P4RT_ARGS+=" --ca_certificate_file=${CA_CRT}"
        readonly CRL=$(echo ${X509} | jq -r '.cert_crl_dir // empty')
        if [ ! -z "$CRL" ]; then
            P4RT_ARGS+=" --cert_crl_dir=${CRL}"
        fi
    fi
else
    P4RT_ARGS+=" --use_insecure_server_credentials"
fi

# Try to read P4RT authorization config from ConfigDB.
readonly AUTHZ_FILE=$(echo ${P4RT} | jq -r '.authz_policy // empty')
if [ ! -z "${AUTHZ_FILE}" ]; then
    P4RT_ARGS+=" --authz_policy_enabled --authorization_policy_file=${AUTHZ_FILE}"
fi

# Try to read P4RT port config from ConfigDB.
readonly PORT=$(echo ${P4RT} | jq -r '.port // empty')
if [ ! -z "${PORT}" ]; then
    P4RT_ARGS+=" --p4rt_grpc_port=${PORT}"
fi

# Try to read P4RT genetlink config from ConfigDB.
readonly GENETLINK=$(echo ${P4RT} | jq -r '.use_genetlink // empty')
if [ ! -z "${GENETLINK}" ]; then
    P4RT_ARGS+=" --use_genetlink=${GENETLINK}"
fi

# Try to read P4RT port ID config from ConfigDB.
readonly PORT_ID=$(echo ${P4RT} | jq -r '.use_port_ids // empty')
if [ ! -z "${PORT_ID}" ]; then
    P4RT_ARGS+=" --use_port_ids=${PORT_ID}"
fi

# Try to read P4RT save forwarding config from ConfigDB.
readonly SAVE_FORWARDING_CONFIG=$(echo ${P4RT} | jq -r '.save_forwarding_config_file // empty')
if [ ! -z "${SAVE_FORWARDING_CONFIG}" ]; then
    P4RT_ARGS+=" --save_forwarding_config_file=${SAVE_FORWARDING_CONFIG}"
fi

# Try to read P4RT unix socket config from ConfigDB.
readonly UNIX_SOCKET=$(echo ${P4RT} | jq -r '.p4rt_unix_socket // empty')
if [ ! -z "${UNIX_SOCKET}" ]; then
    P4RT_ARGS+=" --p4rt_unix_socket=${UNIX_SOCKET}"
fi

exec /usr/local/bin/p4rt ${P4RT_ARGS}
