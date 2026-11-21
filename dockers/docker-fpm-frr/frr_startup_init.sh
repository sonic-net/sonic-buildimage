#!/usr/bin/env bash

_get_frr_restart_enable()
{
    local value
    if ! value=$(sonic-db-cli STATE_DB hget "$1" enable); then
        logger -p error "Failed to read '$1' from STATE_DB, assuming it is not enabled"
    fi
    echo "$value"
}

# Initialize environment consumed by the shared FRR supervisor configuration.
# Keep this function usable by images, such as docker-sonic-vs, that include the
# FRR supervisor fragment but use a different container entrypoint.
initialize_frr_startup_environment()
{
    local system_warm_start="false"
    local bgp_warm_start="false"
    local system_fast_reboot="false"

    # docker-sonic-vs test neighbors have no warm/fast-reboot startup phase and
    # start before their monolithic supervisor brings STATE_DB online.
    if [[ "${1:-}" != "--normal-start" ]]; then
        system_warm_start=$(_get_frr_restart_enable "WARM_RESTART_ENABLE_TABLE|system")
        bgp_warm_start=$(_get_frr_restart_enable "WARM_RESTART_ENABLE_TABLE|bgp")
        system_fast_reboot=$(_get_frr_restart_enable "FAST_RESTART_ENABLE_TABLE|system")
    fi

    # This value reflects the reboot state and rendered FRR config at startup.
    BGPD_GRACEFUL_RESTART_ARG=""
    if grep -qsE '^\s*bgp graceful-restart\s*$' /etc/frr/bgpd.conf /etc/frr/frr.conf &&
       { [[ "$system_warm_start" == "true" ]] ||
         [[ "$system_fast_reboot" == "true" ]] ||
         [[ "$bgp_warm_start" == "true" ]]; }; then
        BGPD_GRACEFUL_RESTART_ARG="-K"
    fi
    export BGPD_GRACEFUL_RESTART_ARG
}
