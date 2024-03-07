#!/bin/bash

# This git has nested submodules whose remotes can change as development
# extends into them.  E.g. they often change from https://github.com/Azure/
# to gitlabsr.nuq.ion.nokia.net requiring sync/update for each nest level.

set -e
prog=$(basename $0)
die () {
  echo "Error near line $prog:${1}"
  exit 1
}
trap 'die ${LINENO}' ERR

git submodule deinit -f --all
rm -fr .git/modules/src/sonic-utilities/
rm -fr .git/modules/platform/broadcom/sonic-platform-modules-nokia/
# sync/update 1st level subs
git submodule sync
git submodule update --init
# sync/update 2nd level subs
git submodule sync --recursive
git submodule update --init --recursive
#
git submodule foreach --recursive git reset --hard

exit $?


