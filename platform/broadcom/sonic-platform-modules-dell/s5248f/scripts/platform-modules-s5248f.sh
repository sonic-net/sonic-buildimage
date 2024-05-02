#!/bin/bash

function platform-modules-s5248fServicePreStop()
{
    /usr/local/bin/s5248f_platform.sh media_down
}
