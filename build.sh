#!/bin/bash
prog=$(basename $0)

usage()
{
  echo -e "
Build sonic

Usage: $prog [OPTIONS]

OPTIONS:
  -h, --help            ; this screen
  -c, --configure       ; configure only
      --rpc             ; build syncd rpc image only
"
  exit $1
}

###################################
# getopt ...
# -h | --help
# -r | --reqarg=foo
# -o | --optarg[=bar]
# getopt -o hr:o:: -l help,reqarg:,optarg:: -n $prog -- "$@"
TEMP=`
getopt \
-o hc \
-l help,configure,rpc \
--name="$prog" \
-- "$@" \
`
if [ $? != 0 ] ; then usage 1 ; fi
eval set -- "$TEMP"

#echo "$TEMP"
###################################
# default options
opt_configure=0
opt_rpc=0
debug=echo

while true; do
  case "$1" in
    -h | --help )      usage 0 ;;
    -c | --configure)  opt_configure=1; shift ;;
         --rpc )       opt_rpc=1; shift ;;
    --        )        shift;  break ;;
    *         )        break ;;
  esac
done

set -e

source nokia-env

mach=$(uname -m)

# check for armhf 32-bit docker
if (docker info --format '{{json .Architecture}}' | grep -q armv8l); then
  mach=armhf
fi

# add remote github user repo for cherry-pick
# arg1: submodule path
# arg2: friendly name
# arg3: remote url
# arg4: commitid
_submodule_add() {
  echo "$prog: submodule change in \"$1\" user \"$2\" cherry-picks \"$4\" from \"$3\""
  pushd $1 &>/dev/null
  if [ -n "${CI_JOB_ID}" ]; then
    # pipeline builds
    git config user.email "${GITLAB_USER_EMAIL}"
    git config user.name "${GITLAB_USER_LOGIN}"
  fi
  # # git revert and/or cherry-picks here
  git remote remove $2 || true
  git remote add -f $2 $3
  git fetch --all
  git cherry-pick --keep-redundant-commits -x $4 || git cherry-pick --abort
  popd &>/dev/null
}

submodule_prs () {
  echo ""
  # Use this to cherry-pick PRs that have not yet been merged to github.
  # args for submodule cherry-picks here: <directory> <user> <https-url> <commit-id>
  _submodule_add src/sonic-swss-common mlok https://github.com/mlok-nokia/sonic-swss-common.git ed4137c7a2cb
}

make init

configure()
{
  case $mach in
    x86_64)
      make PLATFORM=broadcom configure
      ;;
    armhf)
      make PLATFORM=marvell PLATFORM_ARCH=armhf configure
      ;;
    aarch64)
      make PLATFORM=marvell PLATFORM_ARCH=arm64 configure
      ;;
  esac
}

build()
{
  case $mach in
    x86_64)
      submodule_prs
      make target/sonic-broadcom.bin || \
        ( rm -f target/*.bin && make target/sonic-broadcom.bin )
      ;;
    armhf)
      make target/sonic-marvell-armhf.bin || \
        ( rm -f target/*.bin && make target/sonic-marvell-armhf.bin )
      ;;
    aarch64)
      make target/sonic-marvell-arm64.bin || \
        ( rm -f target/*.bin && make target/sonic-marvell-arm64.bin )
      ;;
  esac

}

build_rpc () {
  submodule_prs
  make SONIC_BUILD_JOBS=2 ENABLE_SYNCD_RPC=y target/docker-syncd-brcm-dnx-rpc.gz
}

configure
rc=$?
if [ "$CI_JOB_STAGE" = "configure" -o $opt_configure -eq 1 ]; then
	exit $rc
fi

if [ $opt_rpc -eq 1 ]; then
  build_rpc
  exit $?
fi

build
