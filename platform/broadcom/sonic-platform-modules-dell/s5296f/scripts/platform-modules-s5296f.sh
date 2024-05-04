#!/bin/bash

function platform-modules-s5296fServicePreStop()
{
    /usr/local/bin/s5296f_platform.sh media_down
}
