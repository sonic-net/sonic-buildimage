#!/bin/bash
PRODUCTNAME_PATH="/etc/device/.productname"
BOARDID_PATH="/etc/device/.board_id"
SUBVERSION_PATH="/etc/device/.subversion"

process_file() {
    local file_path="$1"

    # Check if the file exists
    if [ ! -f "$file_path" ]; then
        echo "Error: File '$file_path' not found!"
        return 2
    fi

    # Read the file content
    local content
    content=$(< "$file_path")

    # File exists but is empty
    if [ -z "$content" ]; then
        return 1
    fi

    # Successfully retrieved content
    echo "$content"
    return 0
}

# get productname
PRODUCTNAME_TMP=$(process_file "$PRODUCTNAME_PATH")
get_productname_result=$?
if [ $get_productname_result -ne 0 ]; then
    echo "Failed to get productname, exit."
    exit 1
fi
PRODUCTNAME=$(echo "$PRODUCTNAME_TMP" | tr 'a-z' 'A-Z' | tr '_' '-')

# get board_id
BOARDID=$(process_file "$BOARDID_PATH")
get_boardid_result=$?
if [ $get_boardid_result -ne 0 ]; then
    echo "Failed to get board_id, exit."
    exit 1
fi

# get subversion
SUBVERSION=$(process_file "$SUBVERSION_PATH")
get_subversion_result=$?

if [[ $get_subversion_result -eq 0 ]]; then
    bsp_cmd="/usr/local/bin/$PRODUCTNAME-$BOARDID-$SUBVERSION-platform.sh"
    # Check if the constructed file path exists
    if [[ ! -f "$bsp_cmd" ]]; then
        echo "$bsp_cmd does not exist, Using fallback command."
        bsp_cmd="/usr/local/bin/$BOARDID-$SUBVERSION-platform.sh"
    fi

    if [[ ! -f "$bsp_cmd" ]]; then
        echo "$bsp_cmd does not exist, Using fallback command."
        bsp_cmd="/usr/local/bin/$PRODUCTNAME-$BOARDID-platform.sh"
    fi

    if [[ ! -f "$bsp_cmd" ]]; then
        echo "$bsp_cmd does not exist, Using fallback command."
        bsp_cmd="/usr/local/bin/$BOARDID-platform.sh"
    fi
else
    bsp_cmd="/usr/local/bin/$PRODUCTNAME-$BOARDID-platform.sh"
    # Check if the constructed file path exists
    if [[ ! -f "$bsp_cmd" ]]; then
        echo "$bsp_cmd does not exist, Using fallback command."
        bsp_cmd="/usr/local/bin/$BOARDID-platform.sh"
    fi
fi


if [[ ! -f "$bsp_cmd" ]]; then
    echo "platform init script $bsp_cmd not found, skip."
    exit 0
fi

echo "running platform driver init $bsp_cmd"
eval $bsp_cmd
exit 0
