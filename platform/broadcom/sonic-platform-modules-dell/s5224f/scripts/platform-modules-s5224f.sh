#!/bin/bash

function platform-modules-s5224fServicePreStop()
{
    /usr/local/bin/s5224f_platform.sh media_down
}
