#!/bin/bash

function platform-modules-e3224fServicePreStop()
{
    /usr/local/bin/e3224f_platform.sh media_down
}
