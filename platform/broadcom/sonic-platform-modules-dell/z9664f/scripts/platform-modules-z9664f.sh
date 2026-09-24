#!/bin/bash

function platform-modules-z9664fServicePreStop()
{
    /usr/local/bin/z9664f_platform.sh media_down
}
