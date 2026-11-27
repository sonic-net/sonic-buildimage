#!/bin/bash
# check_containers.sh — Check critical SONiC containers are running
# Exit 0 = all OK, Exit 1 = one or more critical containers down
# stdout: lists down containers

CRITICAL_CONTAINERS="syncd swss bgp teamd pmon database lldp"
DOWN=""

for c in $CRITICAL_CONTAINERS; do
    if ! docker inspect --format='{{.State.Running}}' "$c" 2>/dev/null | grep -q 'true'; then
        DOWN="${DOWN} ${c}"
    fi
done

if [ -n "$DOWN" ]; then
    echo "Down:${DOWN}"
    exit 1
fi

exit 0
