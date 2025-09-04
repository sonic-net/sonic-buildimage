#!/usr/bin/env bash

while true; do
  STATE=$(redis-cli -n 4 HGET "FEATURE|telemetry" state)
  if [ "$STATE" == "enabled" ]; then
    echo "Telemetry is enabled. Starting service..."
    break
  else
    echo "Telemetry is disabled. Entering idle..."
    sleep 10
  fi
done

exec /usr/local/bin/supervisord
