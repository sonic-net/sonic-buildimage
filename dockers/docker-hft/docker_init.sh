#!/usr/bin/env bash
#
# Entrypoint of the hft container.
#
# Arguments given to the container (Docker CMD, "docker run <image> ARG...", or
# the "args" of a Kubernetes container spec) are countersyncd arguments.
# supervisord expands HFT_CONTAINER_ARGS in the countersyncd command and splits
# it the way a shell does, so every argument is quoted to stay intact. Each time
# supervisord (re)starts countersyncd, /usr/bin/countersyncd.sh gets them as its
# own arguments.
#
# Options which only print and exit, such as --help, run countersyncd directly.

for arg in "$@"; do
    case "${arg}" in
        --)
            break
            ;;
        -h|--help|-V|--version)
            exec "${HFT_COUNTERSYNCD_BIN:-/usr/bin/countersyncd}" "$@"
            ;;
    esac
done

HFT_CONTAINER_ARGS=$(python3 -c 'import shlex, sys; print(shlex.join(sys.argv[1:]))' "$@")
export HFT_CONTAINER_ARGS

exec /usr/local/bin/supervisord
