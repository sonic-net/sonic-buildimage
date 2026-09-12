#!/bin/bash

# Ensure a clean state by removing pmon container and its hardware mounts
function clean_pmon() {
    local max_retries=15
    local count=0

    while [ $count -lt $max_retries ]; do
        if timeout 3 docker info > /dev/null 2>&1; then
            break
        fi
        sleep 1
        ((count++))
    done

    if [ $count -eq $max_retries ]; then
        echo "Docker socket timed out!"
        return 1
    fi

    docker rm -f pmon > /dev/null 2>&1
}

clean_pmon
echo "PDDF/PLATFORM/CONFIG pre-init completed"
