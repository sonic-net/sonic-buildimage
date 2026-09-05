#!/bin/bash

platform="$(sonic-cfggen -H -v DEVICE_METADATA.localhost.platform)"
TMP_DIR="/tmp/hw-mgmt-dump"
rm -rf $TMP_DIR
mkdir $TMP_DIR

save_cmd(){
    TOUT=$1
    CMD=$2
    CMD_NAME=$3

    echo "##### $CMD_NAME #####">> $TMP_DIR/hw-mgmt-dump.txt
    eval timeout $TOUT $CMD >> $TMP_DIR/hw-mgmt-dump.txt
}

tar_file(){
    tar czf /tmp/hw-mgmt-dump.tar.gz -C $TMP_DIR .
    rm -rf $TMP_DIR
}

