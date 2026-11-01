#!/bin/bash

start() {
    BDIR="/usr/share/sonic/device"
    CURDEV="$(cat /host/machine.conf | grep onie_platform)"
    DB_CONF_FILE="/var/run/redis/sonic-db/database_config.json"
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
    
    echo "Waiting for $DB_CONF_FILE to be ready..."
    WAIT_TIME=0
    MAX_WAIT=10
    while [ ! -f "$DB_CONF_FILE" ] && [ $WAIT_TIME -lt $MAX_WAIT ]; do
        sleep 1
        WAIT_TIME=$((WAIT_TIME + 1))
        echo "Waiting... ${WAIT_TIME}s / ${MAX_WAIT}s"
    done
    
    if [ -f "$DB_CONF_FILE" ]; then
        echo "$DB_CONF_FILE found. Proceeding..."
    else
        echo "Warning: $DB_CONF_FILE not found after ${MAX_WAIT} seconds, continuing anyway..."
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
    
    echo "Creating symlink: $hwsku_dir -> $target_path"
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
    
    echo "Starting /usr/local/bin/saphy..."
    sleep 1
    /usr/local/bin/saphy service
}

wait() {
    echo "wait /usr/local/bin/saphy..."
}

stop() {
    echo "Stopping /usr/local/bin/saphy..."
    pkill -f "/usr/local/bin/saphy" 2>/dev/null
    echo "Stopped /usr/local/bin/saphy..."
    exit 0
}

case "$1" in
    start|wait|stop)
        $1
        ;;
    *)
        echo "Usage: $0 {start|wait|stop}"
        exit 1
        ;;
esac
