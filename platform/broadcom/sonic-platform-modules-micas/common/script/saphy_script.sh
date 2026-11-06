#!/bin/bash

# Get eth0 MAC address
get_eth0_mac() {
    local mac_file="/sys/class/net/eth0/address"
    
    if [ ! -f "$mac_file" ]; then
        echo "Cannot find $mac_file"
        return 1
    fi
    
    local mac=$(cat "$mac_file" | tr -d '\n')
    
    # Validate MAC address format (XX:XX:XX:XX:XX:XX)
    if ! echo "$mac" | grep -qE '^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$'; then
        echo "Invalid MAC address format: $mac"
        return 1
    fi
    
    echo "$mac"
    return 0
}

# Add n to MAC address
# Parameters: mac_address increment
# Example: mac_add "00:11:22:33:44:55" 3 -> "00:11:22:33:44:58"
mac_add() {
    local mac=$1
    local inc=$2
    
    # Remove colons and convert to uppercase
    local mac_hex=$(echo "$mac" | tr -d ':' | tr 'a-f' 'A-F')
    
    # Convert hex to decimal
    local mac_dec=$((16#$mac_hex))
    
    # Add increment
    local new_mac_dec=$((mac_dec + inc))
    
    # Check overflow (MAC address is 48-bit, max value 2^48 - 1 = 281474976710655)
    local max_mac=281474976710655
    if [ $new_mac_dec -gt $max_mac ]; then
        echo "MAC address overflow when adding $inc"
        return 1
    fi
    
    # Convert back to hex, pad to 12 digits
    local new_mac_hex=$(printf "%012X" $new_mac_dec)
    
    # Insert colon every 2 characters, convert to lowercase
    local new_mac=$(echo "$new_mac_hex" | sed 's/\(..\)/\1:/g' | sed 's/:$//' | tr 'A-F' 'a-f')
    
    echo "$new_mac"
    return 0
}

# Wait for interface to be ready
# Parameters: interface_name timeout_seconds
wait_for_interface() {
    local iface=$1
    local timeout=${2:-60}  # Default timeout 60 seconds
    local waited=0
    
    while [ $waited -lt $timeout ]; do
        # Check if interface exists
        if [ -d "/sys/class/net/$iface" ]; then
            # Check if interface has operstate (optional)
            local operstate=$(cat "/sys/class/net/$iface/operstate" 2>/dev/null)
            if [ -n "$operstate" ]; then
                return 0
            fi
        fi
        sleep 1
        waited=$((waited + 1))
        if [ $((waited % 5)) -eq 0 ]; then
            echo "Waiting for interface $iface to be ready... ${waited}s / ${timeout}s"
        fi
    done
    
    echo "Timeout waiting for interface $iface (${timeout} seconds)"
    return 1
}

# Set interface MAC address
set_interface_mac() {
    local iface=$1
    local mac=$2
    
    # Bring interface down first
    ip link set dev "$iface" down 2>/dev/null
    
    # Set MAC address
    if ip link set dev "$iface" address "$mac" 2>/dev/null; then
        
        # Bring interface back up
        ip link set dev "$iface" up 2>/dev/null
        return 0
    else
	    # Bring interface back up
        ip link set dev "$iface" up 2>/dev/null
        echo "Failed to set $iface MAC address"
        return 1
    fi
}

# set mac function
set_switch_mac() {
    local -n ref_ifaces=$1
    local base_mac
    local new_mac
    local actual_mac

    echo "=== Starting to set MAC address ==="

    # 1. Get eth0 MAC address
    base_mac=$(get_eth0_mac)
    if [ -z "$base_mac" ]; then
        echo "Failed to get eth0 MAC address"
        return 1
    fi

    echo "eth0 MAC address: $base_mac"

    local item
    local iface
    local inc
    local source_port

    # 3. Copy CINT file to syncd
    docker cp /usr/share/sonic/platform/set_bcm_mgmt_mac.config syncd:/set_bcm_mgmt_mac.c

    # 4. Configure each interface
    for item in "${ref_ifaces[@]}"; do

        # Parse:
        # Ethernet1024:4:268
        # iface    inc source_port
        iface="${item%%:*}"
        local tmp="${item#*:}"
        inc="${tmp%%:*}"
        source_port="${tmp##*:}"

        if ! wait_for_interface "$iface" 300; then
            echo "Interface $iface is not ready"
            return 1
        fi

        # Calculate MAC for this interface
        new_mac=$(mac_add "$base_mac" "$inc")
        if [ -z "$new_mac" ]; then
            echo "Failed to calculate MAC for $iface"
            return 1
        fi

        echo "========================================"
        echo "Configuring $iface"
        echo "MAC increment : +$inc"
        echo "MAC address   : $new_mac"
        echo "SourcePort    : $source_port"
        echo "========================================"

        # 5. Add BCM L2 station
        bcmcmd "l2 station add MACaddress=$new_mac IPv4=true IPv6=true ArpRarp=true MACaddressMask=ff:ff:ff:ff:ff:ff SourcePort=$source_port SourcePortMask=0xff" 2>/dev/null;

        # 6. Run CINT for this interface
        bcmcmd "cint set_bcm_mgmt_mac.c -m $new_mac";

        # 7. Set Linux interface MAC
        if ! set_interface_mac "$iface" "$new_mac"; then
            echo "Failed to set MAC for $iface"
            return 1
        fi

        # 8. Verify
        actual_mac=$(cat "/sys/class/net/$iface/address" 2>/dev/null)

        echo "$iface actual MAC address: $actual_mac"

        if [ "$actual_mac" != "$new_mac" ]; then
            echo "Warning: $iface MAC verification failed"
            echo "Expected: $new_mac"
            echo "Actual:   $actual_mac"
            return 1
        fi

        echo "$iface MAC setup successfully"
    done

    echo "========================================"
    echo "=== Switch mgmt MAC setup successful ==="
    echo "========================================"

    return 0
}

wait_syncd_ready()
{
    local timeout=180
    local waited=0
    while [ $waited -lt $timeout ]; do
        if docker inspect syncd >/dev/null 2>&1; then
            status=$(docker inspect --format '{{.State.Running}}' syncd 2>/dev/null)
            if [ "$status" = "true" ]; then
                echo "syncd container is running"
                return 0
            fi
        fi
        echo "Waiting for syncd container ready ${waited}s..."
        sleep 1
        waited=$((waited+1))
    done
    echo "syncd container timeout, abort saphy start"
    return 1
}


start() {
    wait_syncd_ready
    if [ $? -ne 0 ]; then
        exit 1
    fi
    BDIR="/usr/share/sonic/device"
    CURDEV="$(cat /host/machine.conf | grep onie_platform)"
    array=(${CURDEV//=/ })
    PLTF=${array[1]}
    DEVDIR=${BDIR}"/"${PLTF}
    def_sku=${DEVDIR}"/default_sku"
    cur_sku=${DEVDIR}"/current_sku"
    SKU=""
    
    if [ ! -e "/usr/local/bin/saphy" ]; then
        echo "Error: /usr/local/bin/saphy not found"
        exit 0
    fi
    
    if type config-hwsku.sh >/dev/null 2>&1; then
        SKU=""
        RETRY_COUNT=0
        MAX_RETRIES=10
        
        echo "Trying to get SKU via config-hwsku.sh -p..."
        while [ -z "$SKU" ] && [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
            SKU="$(config-hwsku.sh -p 2>/dev/null | xargs)"
            if [ -n "$SKU" ]; then
                echo "config-hwsku.sh get sku: ${SKU}"
                break
            fi
            RETRY_COUNT=$((RETRY_COUNT + 1))
            if [ $RETRY_COUNT -lt $MAX_RETRIES ]; then
                echo "Failed to get SKU (attempt ${RETRY_COUNT}/${MAX_RETRIES}), retrying in 1 second..."
                sleep 1
            fi
        done
        
        if [ -z "$SKU" ]; then
            echo "Warning: Failed to get SKU via config-hwsku.sh after ${MAX_RETRIES} attempts"
        fi
    fi
    
    if [ -z "$SKU" ]; then
        echo "Falling back to file-based SKU detection..."
        if test -e "$cur_sku"; then
            sku=${cur_sku}
            echo "cur_sku: ${sku}"
        elif test -e "$def_sku"; then
            sku=${def_sku}
            echo "def_sku: ${sku}"
        else
            echo "Error: sku file not found !!!"
            exit 1
        fi
        t="$(cat ${sku})"
        array=(${t// / })
        SKU=${array[0]}
        echo "Got SKU from file: ${SKU}"
    fi
    
    if [ -z "$SKU" ]; then
        echo "Error: Failed to get SKU from any source"
        exit 1
    fi
    
	case "${SKU}" in
        "M2-W6950-O128-4X10G")
            echo "SKU:${SKU}, use 1025~1028 ifaces"
            ifaces=(
                "Ethernet1025:4:268"
                "Ethernet1026:5:52"
                "Ethernet1027:6:340"
                "Ethernet1028:7:556"
            )
            ;;
        "M2-W6950-O128-4X25G")
            echo "SKU:${SKU}, use 1025~1028 ifaces"
            ifaces=(
                "Ethernet1025:4:268"
                "Ethernet1026:5:52"
                "Ethernet1027:6:340"
                "Ethernet1028:7:556"
            )
            ;;
        *)
            echo "SKU:${SKU}, use Ethernet1024 ifaces"
            ifaces=(
                "Ethernet1024:4:268"
            )
            ;;
    esac
	
    hwsku_dir="/usr/share/sonic/hwsku"
    target_path="${BDIR}/${PLTF}/${SKU}"
    
    if [ ! -d "$target_path" ]; then
        echo "Error: Target path not found: $target_path"
        exit 1
    fi
    
    if [ -e "$hwsku_dir" ] || [ -L "$hwsku_dir" ]; then
        echo "Removing existing $hwsku_dir"
        rm -rf "$hwsku_dir"
    fi
    
    echo "Creating symlink: "
	echo "$hwsku_dir -> "
	echo "$target_path"
    ln -s "$target_path" "$hwsku_dir"
    
    if [ ! -L "$hwsku_dir" ]; then
        echo "Error: Failed to create symlink"
        exit 1
    fi
    
    platform_dir="/usr/share/sonic/platform"
    target_platform_path="${BDIR}/${PLTF}"
    
    if [ -e "$platform_dir" ] || [ -L "$platform_dir" ]; then
        echo "Removing existing $platform_dir"
        rm -rf "$platform_dir"
    fi
    
    ln -s "$target_platform_path" "$platform_dir"

    echo "SAPHY: before set_switch_mac"
    set_switch_mac ifaces
    echo "SAPHY: before saphy"
	
    echo "Starting /usr/local/bin/saphy ..."
    exec /usr/local/bin/saphy
	
}

wait() {
    echo "wait /usr/local/bin/saphy..."
}

case "$1" in
    start|wait)
        $1
        ;;
    *)
        echo "Usage: $0 {start|wait}"
        exit 1
        ;;
esac
