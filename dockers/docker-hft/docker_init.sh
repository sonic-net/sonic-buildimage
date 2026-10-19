#!/usr/bin/env bash
#
# Entrypoint of the hft container.
#
# Arguments given to the container (Docker CMD, "docker run <image> ARG...", or
# the "args" of a Kubernetes container spec) are forwarded to countersyncd.
# supervisord cannot hand them over to its programs, so they are saved here,
# NUL-separated to keep every argument intact, and picked up by
# /usr/bin/countersyncd.sh each time countersyncd is (re)started.

HFT_CONTAINER_ARGS_FILE=${HFT_CONTAINER_ARGS_FILE:-/var/run/hft/container_args}

mkdir -p "$(dirname "${HFT_CONTAINER_ARGS_FILE}")"
if [[ $# -gt 0 ]]; then
    printf '%s\0' "$@" > "${HFT_CONTAINER_ARGS_FILE}"
else
    : > "${HFT_CONTAINER_ARGS_FILE}"
fi

exec /usr/local/bin/supervisord
