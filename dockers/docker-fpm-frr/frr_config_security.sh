#!/usr/bin/env bash

prepare_frr_config_files()
{
    local config_dir="$1"
    shift
    local config_file path link_count

    for config_file in "$@"; do
        path="${config_dir}/${config_file}"
        if [ -L "${path}" ]; then
            rm -f -- "${path}" || {
                echo "Failed to remove FRR configuration link: ${path}" >&2
                return 1
            }
        elif [ -e "${path}" ]; then
            if [ ! -f "${path}" ]; then
                echo "FRR configuration path is not a regular file: ${path}" >&2
                return 1
            fi
            link_count=$(stat -c %h -- "${path}") || return 1
            if [ "${link_count}" -gt 1 ]; then
                rm -f -- "${path}" || return 1
            fi
        fi
    done
}
