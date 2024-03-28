#!/bin/bash

function remove_target {
    local status="finished"
    [[ ! -z "${2}" ]] &&  status="cached "
    echo "[ ${status} ] [ $1 ] "
}

function add_target {
    echo "[ building ] [ $1 ] "
}

# $3 takes the DPKG caching argument, if the target is loaded from cache,
# it adds the log as 'cached' else it is logged as 'finished'
#
# Without DPKG cache support :
#   [ building ] [ target/docker-base.gz ]
#   [ finished ] [ target/docker-base.gz ]
#
# With DPKG cache support :
#   [ building ] [ target/docker-base.gz ]
#   [ cached   ] [ target/docker-base.gz ]

while getopts ":a:d:e:" opt; do
    case $opt in
        a)
            add_target ${OPTARG}
            ;;
        d)
            remove_target ${OPTARG} $3
            ;;
        e)
            remove_target ${OPTARG} $3
            echo "[ FAIL LOG START ] [ ${OPTARG} ]"
            cat ${OPTARG}.log
            echo "[  FAIL LOG END  ] [ ${OPTARG} ]"
            ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            rm -f .screen
            exit 1
            ;;
        :)
            echo "Option -$OPTARG requires an argument." >&2
            rm -f .screen
            exit 1
            ;;
    esac
done
