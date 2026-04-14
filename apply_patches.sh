#!/bin/bash

# Enable 'set -e' to exit immediately if a command exits with a non-zero status
set -e

PATCH_DIR="wistron_patches"
ZTP_DIR="$PATCH_DIR/ztp_workaround"
ZTP_ACTION="disable"

# Parse arguments for simpler execution
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -z|--ztp|ZTP=yes) ZTP_ACTION="enable" ;;
        ZTP=no) ZTP_ACTION="revert" ;;
        -h|--help) 
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -z, --ztp, ZTP=yes    Enable ZTP workaround patches (applied before regular patches)"
            echo "  ZTP=no                Revert ZTP workaround patches if applied, and skip them"
            echo "  -h, --help            Show this help message"
            echo "  (Default: Skip ZTP workaround patches without reverting)"
            exit 0
            ;;
        *) echo "[ERROR] Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

echo "[INFO] Starting to apply patches..."

# 1. Check if the patch directory exists
if [ ! -d "$PATCH_DIR" ]; then
    echo "[ERROR] Patch directory not found: $PATCH_DIR"
    exit 1
fi

# Function to apply a single patch
apply_patch_file() {
    local patch_file="$1"

    # Check if the patch is already applied by doing a dry-run in reverse (-R).
    # If this succeeds, the patch is already present.
    if patch -p1 -R --dry-run < "$patch_file" >/dev/null 2>&1; then
        echo "[INFO] SKIP: $patch_file (Already applied)"
        echo "----------------------------------------"
        return 0
    fi
    
    echo "[ACTION] APPLYING: $patch_file"
    # Execute the patch command; trigger the else block if it fails
    if patch -p1 < "$patch_file"; then
        echo "[SUCCESS] DONE: $patch_file"
        echo "----------------------------------------"
    else
        echo "[ERROR] FAILED: $patch_file"
        echo "[DEBUG] Please check the patch file contents or resolve conflicts."
        exit 1
    fi
}

# Function to revert a single patch
revert_patch_file() {
    local patch_file="$1"

    # Check if the patch is already applied by doing a dry-run in reverse (-R).
    # If this succeeds, the patch is already present, meaning we can revert it.
    if patch -p1 -R --dry-run < "$patch_file" >/dev/null 2>&1; then
        echo "[ACTION] REVERTING: $patch_file"
        if patch -p1 -R < "$patch_file"; then
            echo "[SUCCESS] REVERTED: $patch_file"
            echo "----------------------------------------"
        else
            echo "[ERROR] FAILED TO REVERT: $patch_file"
            exit 1
        fi
    else
        # Not currently applied, so no need to revert
        echo "[INFO] SKIP REVERT: $patch_file (Not currently applied)"
        echo "----------------------------------------"
    fi
}

shopt -s nullglob

# 2. Handle ZTP patches based on action
if [ "$ZTP_ACTION" == "enable" ]; then
    echo "[INFO] --- ZTP patches enabled ---"
    if [ -d "$ZTP_DIR" ]; then
        ztp_files=("$ZTP_DIR"/*.patch)
        if [ ${#ztp_files[@]} -gt 0 ]; then
            dhcp_patches=()
            other_patches=()
            for patch_file in "${ztp_files[@]}"; do
                case "$patch_file" in
                    *[dD][hH][cC][pP]*) dhcp_patches+=("$patch_file") ;;
                    *) other_patches+=("$patch_file") ;;
                esac
            done

            if [ ${#dhcp_patches[@]} -gt 0 ]; then
                mapfile -t dhcp_patches < <(printf "%s\n" "${dhcp_patches[@]}" | sort -V)
                echo "[INFO] Applying DHCP ZTP patches first..."
                for patch_file in "${dhcp_patches[@]}"; do
                    apply_patch_file "$patch_file"
                done
            fi

            if [ ${#other_patches[@]} -gt 0 ]; then
                mapfile -t other_patches < <(printf "%s\n" "${other_patches[@]}" | sort -V)
                echo "[INFO] Applying remaining ZTP patches..."
                for patch_file in "${other_patches[@]}"; do
                    apply_patch_file "$patch_file"
                done
            fi
        else
            echo "[WARNING] No matching .patch files found in $ZTP_DIR."
        fi
    else
        echo "[WARNING] ZTP workaround directory not found: $ZTP_DIR"
    fi
elif [ "$ZTP_ACTION" == "revert" ]; then
    echo "[INFO] --- Reverting ZTP patches ---"
    if [ -d "$ZTP_DIR" ]; then
        ztp_files=("$ZTP_DIR"/*.patch)
        if [ ${#ztp_files[@]} -gt 0 ]; then
            dhcp_patches=()
            other_patches=()
            for patch_file in "${ztp_files[@]}"; do
                case "$patch_file" in
                    *[dD][hH][cC][pP]*) dhcp_patches+=("$patch_file") ;;
                    *) other_patches+=("$patch_file") ;;
                esac
            done

            # Revert in exact reverse numerical order: remaining patches first, then DHCP patches
            if [ ${#other_patches[@]} -gt 0 ]; then
                mapfile -t other_patches < <(printf "%s\n" "${other_patches[@]}" | sort -rV)
                echo "[INFO] Reverting remaining ZTP patches..."
                for patch_file in "${other_patches[@]}"; do
                    revert_patch_file "$patch_file"
                done
            fi

            if [ ${#dhcp_patches[@]} -gt 0 ]; then
                mapfile -t dhcp_patches < <(printf "%s\n" "${dhcp_patches[@]}" | sort -rV)
                echo "[INFO] Reverting DHCP ZTP patches..."
                for patch_file in "${dhcp_patches[@]}"; do
                    revert_patch_file "$patch_file"
                done
            fi
        else
            echo "[WARNING] No matching .patch files found in $ZTP_DIR."
        fi
    else
        echo "[WARNING] ZTP workaround directory not found: $ZTP_DIR"
    fi
else
    echo "[INFO] --- ZTP patches disabled (uses default behavior, skipped) ---"
fi

echo "[INFO] --- Applying regular patches ---"
# 3. Find regular patches
patch_files=("$PATCH_DIR"/[0-9][0-9][0-9][0-9]-*.patch)

# 4. Check if there are any files matching the pattern
if [ ${#patch_files[@]} -eq 0 ]; then
    echo "[WARNING] No matching .patch files found in $PATCH_DIR."
else
    # Loop through and apply each regular patch
    for patch_file in "${patch_files[@]}"; do
        apply_patch_file "$patch_file"
    done
fi

echo "[INFO] Patch execution finished!"

