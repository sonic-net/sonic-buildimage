#!/bin/bash

# Provide file paths to archive for debug image as relative to src subdir
#
if [ "$DEBUG_IMG" == "y" ]
then
    for i in $DEBUG_SRC_ARCHIVE_DIRS
    do
        find src/$i/ -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -type f
    done | tar -czf $DEBUG_SRC_ARCHIVE_FILE -T -
fi

