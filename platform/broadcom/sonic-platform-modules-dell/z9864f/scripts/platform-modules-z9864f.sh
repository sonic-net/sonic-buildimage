#!/bin/bash

function platform-modules-z9864fServicePreStop()
{
    docker exec -i pmon pkill -15 xcvrd_phy
    sleep 3
    /usr/local/bin/z9864f_platform.sh media_down
}
