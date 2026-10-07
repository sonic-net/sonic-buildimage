#!/bin/bash
# Script to control DPU management traffic forwarding through the SmartSwitch.

command_name=$0
mgmt_iface=eth0
midplane_iface=bridge-midplane
dpu_l=()
declare -A midplane_ip_dict
fw_change=enable

usage(){
    echo "Syntax: $command_name inbound/outbound -e|--enable -d|--disable [--dpus,--ports,--nofwctrl]"
    echo "Arguments:"
    echo "inbound        Control DPU inbound SSH forwarding (destination port 22)"
    echo "outbound       Control DPU outbound traffic forwarding"
    echo "-e|--enable    Enable forwarding"
    echo "-d|--disable   Disable forwarding"
    echo "--dpus         Comma-separated DPU names, or all (inbound only)"
    echo "--ports        One decimal port per DPU, 1024-65535 (inbound only)"
    echo "--nofwctrl     Leave IPv4 forwarding settings unchanged"
    echo "Each selected DPU must have exactly one IPv4 address in CONFIG_DB."
    echo "Leading-zero ports are normalized to decimal. Invalid inputs cause no changes."
}

error(){
    printf 'Error: %s\n' "$*" >&2
}

add_rem_valid_iptable(){
    local op=$1 table=$2 chain=$3 action check_output chain_output status present=0
    shift 3
    local rule=("$@")
    case "$op" in
        enable) action=-A ;;
        disable) action=-D ;;
        *) error "Invalid forwarding operation: $op"; return 1 ;;
    esac
    case "$table:$chain" in
        nat:PREROUTING|nat:POSTROUTING|filter:FORWARD) ;;
        *) error "Invalid forwarding table/chain: $table/$chain"; return 1 ;;
    esac
    if check_output=$(LC_ALL=C iptables -t "$table" -C "$chain" "${rule[@]}" 2>&1); then
        present=1
    else
        status=$?
        if [ "$status" -ne 1 ]; then
            error "Cannot check $table/$chain rule (status $status): $check_output"
            return "$status"
        fi
        # Legacy iptables can use a missing-chain diagnostic for an absent rule.
        case "$check_output" in
            ""|*"Bad rule (does a matching rule exist in that chain?).") ;;
            *"No chain/target/match by that name.")
                if chain_output=$(LC_ALL=C iptables -t "$table" -S "$chain" 2>&1); then
                    :
                else
                    status=$?
                    error "Cannot check $table/$chain chain (status $status): $chain_output"
                    return "$status"
                fi
                ;;
            *)
                error "Cannot check $table/$chain rule (status $status): $check_output"
                return "$status"
                ;;
        esac
    fi
    if [[ "$op" == enable && "$present" -eq 0 ||
          "$op" == disable && "$present" -eq 1 ]]; then
        if iptables -t "$table" "$action" "$chain" "${rule[@]}"; then
            return 0
        else
            status=$?
            error "Cannot $op $table/$chain rule (status $status)"
            return "$status"
        fi
    fi
    printf 'Rule change not required: '
    printf '%q ' iptables -t "$table" "$action" "$chain" "${rule[@]}"
    printf '\n'
}

control_forwarding(){
    local op=$1 value=0
    [ "$op" != enable ] || value=1
    if [ "$fw_change" == enable ]; then
        if ! printf '%s\n' "$value" > /proc/sys/net/ipv4/ip_forward; then
            error "Cannot update global IPv4 forwarding"
            return 1
        fi
        if ! printf '%s\n' "$value" > "/proc/sys/net/ipv4/conf/$mgmt_iface/forwarding"; then
            error "Cannot update $mgmt_iface IPv4 forwarding"
            return 1
        fi
    fi
}

validate_ipv4(){
    local address=$1 octet
    local octets=()
    [[ "$address" =~ ^([0-9]{1,3}\.){3}[0-9]{1,3}$ ]] || return 1
    IFS=. read -r -a octets <<< "$address"
    for octet in "${octets[@]}"; do
        [[ ${#octet} -eq 1 || "$octet" != 0* ]] || return 1
        (( 10#$octet <= 255 )) || return 1
    done
}

normalize_port(){
    local port=$1
    [[ "$port" =~ ^[0-9]+$ ]] || {
        error "Port must contain decimal digits only: $port"
        return 1
    }
    port="${port#"${port%%[!0]*}"}"
    port=${port:-0}
    if [[ ${#port} -gt 5 ]] || (( 10#$port < 1024 || 10#$port > 65535 )); then
        error "Port must be in range 1024-65535: $1"
        return 1
    fi
    printf '%s\n' "$port"
}

port_use_validation(){
    local listeners port
    if ! listeners=$(netstat -tuln); then
        error "Cannot check ports in use"
        return 1
    fi
    for port in "$@"; do
        if awk -v port=":$port" '$4 ~ (port "$") { found=1 } END { exit !found }' <<< "$listeners"; then
            error "Provided port $port is in use by another process"
            return 1
        fi
    done
}

redis_read(){
    local value
    # Older redis-cli returns zero for Redis errors; callers validate response types.
    if ! value=$(redis-cli --raw -n 4 "$@"); then
        error "Cannot read CONFIG_DB $1"
        return 1
    fi
    printf '%s\n' "$value"
}

validate_list(){
    [[ -n "$1" && "$1" != ,* && "$1" != *, && "$1" != *,,* && "$1" != *[[:space:]]* ]]
}

inbound_validation(){
    local keys key dpu found item index midplane_int_name midplane_ip
    dpu_l=()
    midplane_ip_dict=()
    keys=$(redis_read keys 'DPUS|*') || return
    if [ -z "$keys" ]; then
        error "No DPUs detected on device"
        return 1
    fi
    while IFS= read -r key; do
        dpu=${key#DPUS|}
        if [[ "$key" != DPUS\|* || ! "$dpu" =~ ^[a-zA-Z0-9_.-]+$ ]]; then
            error "Invalid DPU key in CONFIG_DB: $key"
            return 1
        fi
        dpu_l+=("$dpu")
    done <<< "$keys"
    if [ "$arg_dpu_names" == all ]; then
        mapfile -t sel_dpu_names < <(printf '%s\n' "${dpu_l[@]}" | sort)
    else
        if ! validate_list "$arg_dpu_names"; then
            error "Provide a nonempty comma-separated DPU list"
            return 1
        fi
        IFS=, read -r -a sel_dpu_names <<< "$arg_dpu_names"
        for dpu in "${sel_dpu_names[@]}"; do
            found=0
            for item in "${dpu_l[@]}"; do
                [ "$item" != "$dpu" ] || found=1
            done
            if [ "$found" -eq 0 ]; then
                error "$dpu is not detected; provide proper DPU names"
                return 1
            fi
        done
    fi
    if ! validate_list "$arg_port_list"; then
        error "Provide a nonempty comma-separated port list"
        return 1
    fi
    IFS=, read -r -a provided_ports <<< "$arg_port_list"
    if [ "${#sel_dpu_names[@]}" -ne "${#provided_ports[@]}" ]; then
        error "DPU count does not match provided port count"
        return 1
    fi
    for index in "${!provided_ports[@]}"; do
        provided_ports[index]=$(normalize_port "${provided_ports[index]}") || return
    done
    port_use_validation "${provided_ports[@]}" || return
    for dpu in "${sel_dpu_names[@]}"; do
        midplane_int_name=$(redis_read hget "DPUS|$dpu" midplane_interface) || return
        if [[ ! "$midplane_int_name" =~ ^[a-zA-Z0-9_.:-]{1,15}$ ]]; then
            error "Invalid or missing midplane interface for $dpu"
            return 1
        fi
        midplane_ip=$(redis_read hget "DHCP_SERVER_IPV4_PORT|$midplane_iface|$midplane_int_name" 'ips@') || return
        if ! validate_ipv4 "$midplane_ip"; then
            error "$dpu requires exactly one canonical IPv4 address in CONFIG_DB"
            return 1
        fi
        midplane_ip_dict["$dpu"]=$midplane_ip
    done
}

ctrl_dpu_ob_forwarding(){
    local op=$1
    control_forwarding "$op" || return
    add_rem_valid_iptable "$op" nat POSTROUTING -o "$mgmt_iface" -j MASQUERADE || return
    add_rem_valid_iptable "$op" filter FORWARD -i "$mgmt_iface" -o "$midplane_iface" -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT || return
    add_rem_valid_iptable "$op" filter FORWARD -i "$midplane_iface" -o "$mgmt_iface" -j ACCEPT || return
    printf '%sd DPU management outbound traffic forwarding\n' "${op^}"
}

ctrl_dpu_ib_forwarding(){
    local op=$1 dest_port=22 index dpu_name dpu_midplane_ip switch_port
    control_forwarding "$op" || return
    add_rem_valid_iptable "$op" filter FORWARD -i "$midplane_iface" -o "$mgmt_iface" -p tcp -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT || return
    add_rem_valid_iptable "$op" filter FORWARD -i "$mgmt_iface" -o "$midplane_iface" -p tcp --dport "$dest_port" -j ACCEPT || return
    for index in "${!sel_dpu_names[@]}"; do
        dpu_name=${sel_dpu_names[$index]}
        dpu_midplane_ip=${midplane_ip_dict[$dpu_name]}
        switch_port=${provided_ports[$index]}
        add_rem_valid_iptable "$op" nat POSTROUTING -p tcp -d "$dpu_midplane_ip" --dport "$dest_port" -j SNAT --to-source "$midplane_gateway" || return
        add_rem_valid_iptable "$op" nat PREROUTING -i "$mgmt_iface" -p tcp --dport "$switch_port" -j DNAT --to-destination "$dpu_midplane_ip:$dest_port" || return
    done
    printf '%sd DPU management inbound traffic forwarding\n' "${op^}"
}

main(){
    local next_operation interface_info
    if [ "$EUID" -ne 0 ]; then
        error "Please run the script with elevated privileges using sudo"
        return 1
    fi
    direction=${1:-}
    case "$direction" in
        inbound|outbound) shift ;;
        *) error "Provide direction inbound or outbound"; usage; return 1 ;;
    esac
    operation=""
    arg_dpu_names=""
    arg_port_list=""
    fw_change=enable
    while [ "$#" -gt 0 ]; do
        case "$1" in
            -e|--enable|-d|--disable)
                next_operation=enable
                [[ "$1" != -d && "$1" != --disable ]] || next_operation=disable
                if [[ -n "$operation" && "$operation" != "$next_operation" ]]; then
                    error "Enable and disable are mutually exclusive"
                    return 1
                fi
                operation=$next_operation
                ;;
            --dpus|--ports)
                if [[ "$direction" != inbound || "$#" -lt 2 || -z "$2" || "$2" == -* ]]; then
                    error "Invalid or missing value for $1 (inbound only)"
                    return 1
                fi
                if [ "$1" == --dpus ]; then
                    arg_dpu_names=$2
                else
                    arg_port_list=$2
                fi
                shift
                ;;
            --nofwctrl) fw_change=disable ;;
            --)
                if [ "$#" -ne 1 ]; then
                    error "Unexpected arguments after --"
                    return 1
                fi
                ;;
            *) error "Invalid argument: $1"; usage; return 1 ;;
        esac
        shift
    done
    if [ -z "$operation" ]; then
        error "Provide operation -e or -d"
        return 1
    fi
    if ! interface_info=$(ifconfig "$midplane_iface" 2>/dev/null); then
        error "$midplane_iface does not exist; run on a SmartSwitch"
        return 1
    fi
    midplane_gateway=$(awk '/inet / {sub(/^addr:/, "", $2); print $2}' <<< "$interface_info")
    if ! validate_ipv4 "$midplane_gateway"; then
        error "Cannot obtain one canonical IPv4 gateway for $midplane_iface"
        return 1
    fi
    case "$direction" in
        inbound)
            inbound_validation || return
            ctrl_dpu_ib_forwarding "$operation"
            ;;
        outbound) ctrl_dpu_ob_forwarding "$operation" ;;
    esac
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    main "$@"
fi
