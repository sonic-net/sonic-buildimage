#!/bin/bash
#########################################################################
## This script is to automate Orignaization specific extensions 	#
## such as Configuration & Scripts for features like AAA, ZTP, etc.	#
## to include in ONIE installer image					#
##									#
## USAGE:								#
##   ./organization_extensions.sh -f<filesystem_root> -n<hostname> 	#
##   ./organization_extensions.sh 	\				#
##			--fsroot <filesystem_root> 	\		#
##			--hostname <hostname>				#
## PARAMETERS:								#
##   -f FILESYSTEM_ROOT							#
##          The location of the root file system			#
##   -h HOSTNAME							#
##          The hostname of the target system				#
#########################################################################

## Initialize the arguments to default values.
## The values get updated to user provided value, if supplied
FILESYSTEM_ROOT=./fsroot
HOSTNAME=sonic

# read the options
TEMP=`getopt -o f:h: --long fsroot:,hostname: -- "$@"`
eval set -- "$TEMP"

# extract options and their arguments into variables.
while true ; do
    case "$1" in
        -f|--fsroot)
            case "$2" in
                "") shift 2 ;;
                *) FILESYSTEM_ROOT=$2 ; shift 2 ;;
            esac ;;
        -h|--hostname)
            case "$2" in
                "") shift 2 ;;
                *) HOSTNAME=$2 ; shift 2 ;;
            esac ;;
        --) shift ; break ;;
        *) echo "Internal error!" ; exit 1 ;;
    esac
done

echo "Executing SONIC Organization Extensions"

## Place your Organization specific code / scipts here ... 


echo "SONIC Organization Extensions - Done"

