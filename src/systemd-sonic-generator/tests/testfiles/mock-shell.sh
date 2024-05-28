#!/usr/bin/bash

OUTPUT_DIR=tests/ssg-test
OUTPUT_FILE=${OUTPUT_DIR}/mock_shell.output

mkdir -p ${OUTPUT_DIR}

# Print output file path if no argument
if [ "$#" -eq 0 ]; then
    echo ${OUTPUT_FILE}
    exit 0
fi

echo ${@:2} | tee -a ${OUTPUT_FILE}
