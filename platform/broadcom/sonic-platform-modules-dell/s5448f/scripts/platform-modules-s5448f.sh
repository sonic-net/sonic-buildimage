#!/bin/bash

function platform-modules-s5448fServicePreStop()
{
    /usr/local/bin/s5448f_platform.sh media_down
}
