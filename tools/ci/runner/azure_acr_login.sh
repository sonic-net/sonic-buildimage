#!/bin/bash
set -euo pipefail

if [[ $# != 1 || -z $1 ]]; then
    echo 'Usage: azure_acr_login.sh REGISTRY_NAME' >&2
    exit 1
fi

: "${AZURE_SUBSCRIPTION_ID:?Set AZURE_SUBSCRIPTION_ID to the ACR subscription}"
: "${REGISTRY_SERVER:?Set REGISTRY_SERVER to the Docker registry hostname}"
: "${REGISTRY_PORT:?Set REGISTRY_PORT to the Docker registry port}"
: "${DOCKER_CONFIG:?Set DOCKER_CONFIG to the job-private Docker configuration directory}"

install -d -m 0700 "$DOCKER_CONFIG"

if ! command -v az > /dev/null; then
    installer=$(mktemp)
    trap 'rm -f "$installer"' EXIT
    curl --fail --location --retry 3 --proto '=https' --tlsv1.2 \
        https://aka.ms/InstallAzureCLIDeb -o "$installer"
    sudo -n bash "$installer"
fi
az version --output none
az login --identity --output none
az account set --subscription "$AZURE_SUBSCRIPTION_ID"
# A pull-only identity can reach the registry without permission to discover
# its login server through Azure Resource Manager. Give the CLI the same
# tenant suffix that Docker uses when it falls back to the public ACR domain.
login_args=(--name "$1")
hostname_label=${REGISTRY_SERVER%%.*}
if [[ $REGISTRY_SERVER == "$hostname_label.azurecr.io" && $hostname_label == "$1-"?* ]]; then
    login_args+=(--suffix "${hostname_label#"$1-"}")
fi
registry_token=$(az acr login "${login_args[@]}" --expose-token --query accessToken --output tsv)
if [[ -z "$registry_token" ]]; then
    echo 'ACR returned an empty access token' >&2
    exit 1
fi
printf '%s' "$registry_token" | docker login "$REGISTRY_SERVER:$REGISTRY_PORT" \
    --username 00000000-0000-0000-0000-000000000000 --password-stdin
