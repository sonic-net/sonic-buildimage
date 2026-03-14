#!/bin/bash
#/*
#**********************************************************************
#*
#* @filename  funcs.sh
#*
#* @purpose   api functions script for fan-ctrl
#*
#* @create    2017/08/09
#*
#* @author    nixon.chu  <nixon.chu@mic.com.tw>
#*
#* @history   2017/08/09: init version
#*
#**********************************************************************
#*/

DIR=$(dirname $0)

function Platform_init() {
    ${DIR}/init.sh
}
