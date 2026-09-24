#!/bin/bash

function platform-modules-s3248tServicePreStop()
{
    /usr/local/bin/s3248t_platform.sh media_down
}
