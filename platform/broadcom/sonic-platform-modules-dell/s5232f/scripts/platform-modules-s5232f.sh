#!/bin/bash

function platform-modules-s5232fServicePreStop()
{
    /usr/local/bin/s5232f_platform.sh media_down
}
