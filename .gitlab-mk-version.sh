#!/bin/bash

set -e

# branchcode to branch
list=" \
  999999,master
  999998,nokia-7215-t1
  999997,nokia-7215-t1-master
  999996,rebase-voq
  999995,msft-ch-drop3
"

branchcode=999999

for v in $list ; do
  a=`echo $v | awk -F, '{print$1}'`
  b=`echo $v | awk -F, '{print$2}'`
  if [ "${CI_COMMIT_BRANCH}" = "$b" ]; then
    branchcode=$a
  fi
done

echo ${branchcode}-${CI_PIPELINE_ID}

exit $?


