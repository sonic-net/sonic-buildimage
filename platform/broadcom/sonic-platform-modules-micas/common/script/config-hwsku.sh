#!/bin/bash
###########################################################################
# SONiC default HwSKU Setup utility                                       #
#                                                                         #
# This script is used to set HWSKU in deafult_sku                         #
#                                                                         #
###########################################################################

# Check for user permissions
check_permissions()
{
    if [ "$EUID" != "0" ]; then
        echo "Root privileges are required for this operation"
        exit 1
    fi
}

# Command usage and help
usage()
{
    cat << EOF
 Usage:  config-hwsku < list | set <hwsku-name> [-y]>
         -l    - Display supported list HWSKU for this platform.
         -s    - Set user provided HwSKU as default SKU.
         -y    - Use this option to skip user confirmation
		 -p    - Display current HWSKU
EOF
}

# Collect all information needed to get list of HWSKU
get_platform_info()
{
    # Initialize variables
    PLATFORM=""
    
    # Method 1: Try sonic-cfggen
    if command -v sonic-cfggen &> /dev/null; then
        # Use printf to ensure clean output and remove any carriage returns
        PLATFORM=$(sonic-cfggen -H -v DEVICE_METADATA.localhost.platform 2>/dev/null | tr -d '\r\n' | xargs)
    fi
    
    # Method 2: Use machine.conf directly (most reliable)
    if [ -z "$PLATFORM" ] && [ -f /host/machine.conf ]; then
        PLATFORM=$(grep "^onie_platform=" /host/machine.conf | cut -d'=' -f2 | tr -d '\r\n' | xargs)
    fi
    
    # Method 3: Use platform.json as fallback
    if [ -z "$PLATFORM" ] && [ -f /etc/sonic/platform.json ]; then
        PLATFORM=$(python3 -c "import json; f=open('/etc/sonic/platform.json'); d=json.load(f); print(d.get('platform', '')); f.close()" 2>/dev/null | tr -d '\r\n' | xargs)
    fi
    
    # Final validation
    if [ -z "$PLATFORM" ]; then
        echo "Error: Cannot determine platform"
        echo "Please check: /host/machine.conf or /etc/sonic/platform.json"
        exit 1
    fi
    
    # Remove any remaining non-printable characters
    PLATFORM=$(echo "$PLATFORM" | tr -cd '[:alnum:][:space:]-_.' | xargs)
    
    # Set platform directory
    PLATFORM_DIR="/usr/share/sonic/device/$PLATFORM"
    
    if [ ! -d "$PLATFORM_DIR" ]; then
        echo "Error: Platform directory not found: $PLATFORM_DIR"
        echo "Available directories:"
        ls -la /usr/share/sonic/device/
        exit 1
    fi
    
    # Get default SKU
    if [ -f "$PLATFORM_DIR/default_sku" ]; then
        # Read and clean the default_sku line
        DEFAULT_SKU_LINE=$(head -n 1 "$PLATFORM_DIR/default_sku" | tr -d '\r\n' | xargs)
        PRESET=($DEFAULT_SKU_LINE)
        if [ ${#PRESET[@]} -ge 2 ]; then
            HWSKU_STR_START=${PRESET[0]:0:4}
            DEFAULT_PRESET=${PRESET[1]}
        else
            HWSKU_STR_START=""
            DEFAULT_PRESET=""
        fi
    else
        HWSKU_STR_START=""
        DEFAULT_PRESET=""
        echo "Warning: No default_sku found in $PLATFORM_DIR"
    fi
    
    # Get current HWSKU
    PRESENT_HWSKU=""
    if command -v sonic-cfggen &> /dev/null; then
        PRESENT_HWSKU=$(sonic-cfggen -d -v 'DEVICE_METADATA["localhost"]["hwsku"]' 2>/dev/null | tr -d '\r\n' | xargs)
    fi
    
    if [ -z "$PRESENT_HWSKU" ] && [ -f /etc/sonic/config_db.json ]; then
        PRESENT_HWSKU=$(python3 -c "import json; f=open('/etc/sonic/config_db.json'); d=json.load(f); print(d.get('DEVICE_METADATA', {}).get('localhost', {}).get('hwsku', '')); f.close()" 2>/dev/null | tr -d '\r\n' | xargs)
    fi
}

# Display HWSKUs supported on current platform
list_hwskus()
{
    echo "List of supported HWSKU:"
    if [ ! -d "$PLATFORM_DIR" ]; then
        echo "Error: Platform directory not found"
        return 1
    fi
    
    for sku in "$PLATFORM_DIR"/*/; do
        if [ -d "$sku" ]; then
            sku="${sku%/}"
            sku="${sku##*/}"
            
            # Skip non-matching prefixes if defined
            if [ -n "$HWSKU_STR_START" ] && [ "$HWSKU_STR_START" != "${sku:0:4}" ]; then
                continue
            fi
            
            if [ "$sku" = "$PRESENT_HWSKU" ]; then
                echo "$sku (Present)"
            else
                echo "$sku"
            fi
        fi
    done
}

# Set requested HWSKU
set_hwsku()
{
    # Parse arguments passed
    HWSKU=$(echo "$1" | tr -d '\r\n' | xargs)
    
    # Validate HWSKU is not empty
    if [ -z "$HWSKU" ]; then
        echo "Error: HWSKU name cannot be empty"
        exit 1
    fi
    
    # Validate in Supported list
    valid_sku=0
    for sku in "$PLATFORM_DIR"/*/; do
        if [ -d "$sku" ]; then
            sku="${sku%/}"
            sku="${sku##*/}"
            
            if [ -n "$HWSKU_STR_START" ] && [ "$HWSKU_STR_START" != "${sku:0:4}" ]; then
                continue
            fi
            
            if [ "$sku" = "$HWSKU" ]; then
                valid_sku=1
                break
            fi
        fi
    done
    
    if [ $valid_sku -eq 1 ]; then
        if [ "$PRESENT_HWSKU" = "$HWSKU" ]; then
            echo "Same as present HWSKU $HWSKU, no changes made"
        else
            echo -e "Warning: This deletes existing configurations and cause switch to reboot."
            if [ "${EXECUTE}" = "no" ]; then
                read -r -p "Are you sure you want to change? [y/N] " response
            else
                response='y'
            fi
            case $response in
                [yY][eE][sS]|[yY])
                    echo "Setting Default HWSKU to $HWSKU $DEFAULT_PRESET"
                    
                    # Write to default_sku
                    if [ -f "$PLATFORM_DIR/default_sku" ]; then
                        echo "$HWSKU $DEFAULT_PRESET" > "$PLATFORM_DIR/default_sku"
                    fi
                    
                    # Update sonic-environment
                    if [ -r /etc/sonic/sonic-environment ]; then
                        sed -i '/HWSKU.*/d' /etc/sonic/sonic-environment
                        echo "HWSKU=$HWSKU" >> /etc/sonic/sonic-environment
                    fi
                    
                    echo "Removing existing config_db.json"
                    if [ -r /etc/sonic/config_db.json ]; then
                        rm -rf /etc/sonic/config_db.json
                    fi
                    
                    # Remove and recreate hwsku directory
                    echo "Deleting hwsku directory..."
                    rm -rf /usr/share/sonic/hwsku
                    
                    echo "Please reboot manually to apply changes"
                    ;;
                *)
                    exit 0
                    ;;
            esac
        fi
    else
        echo "Invalid HWSKU - $HWSKU"
        echo "Available SKUs:"
        list_hwskus
        exit 1
    fi
}

# Main function
main()
{
    # Check permissions first
    check_permissions
    
    # Get platform information
    get_platform_info
    
    local cmd="$1"
    
    if [ "$cmd" = "-p" ]; then
        echo "$PRESENT_HWSKU"
        exit 0
    fi
	
    if [ "$cmd" = "help" ] || \
       [ "$cmd" = "-h" ] || [ "$cmd" = "--help" ]; then
        usage
        exit 0
    fi
    
    # Display supported HWSKU's for current platform
    if [ "$cmd" = "-l" ]; then
        list_hwskus
    # Set user provided HWSKU as default_SKU
    elif [ "$cmd" = "-s" ]; then
        EXECUTE="no"
        if [ "$3" = "-y" ]; then
            EXECUTE="yes"
        elif [ -n "$3" ] && [ "$3" != "-y" ]; then
            usage
            exit 1
        fi
        
        if [ -z "$2" ]; then
            echo "Error: HWSKU name required"
            usage
            exit 1
        fi
        
        set_hwsku "$2"
    # Validate supported commands
    else
        usage
        exit 1
    fi
    
    exit 0
}

# Call main function with all arguments
main "$@"