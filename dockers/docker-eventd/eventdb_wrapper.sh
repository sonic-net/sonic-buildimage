#!/bin/bash

JSON_FILE="/etc/evprofile/default.json"

# Check if file exists and is not empty
if [ -s "$JSON_FILE" ]; then
    # Check if "events" is defined and has at least one entry
    if jq -e '.events and (.events | length > 0)' "$JSON_FILE" > /dev/null; then
        echo "Valid events found. Starting eventdb."
        exec /usr/bin/eventdb
    else
        echo "'events' list is missing or empty. Skipping eventdb start."
        exit 0
    fi
else
    echo "JSON file missing or empty. Skipping eventdb start."
    exit 0
fi
