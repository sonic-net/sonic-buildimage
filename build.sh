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
  -s, --submodule       ; test submodule patch function
      --rpc-dnx         ; build dnx syncd rpc image only
      --rpc-xgs         ; build xgs syncd rpc image only
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
-o hcs \
-l help,configure,rpc-dnx,rpc-xgs,submodule \
--name="$prog" \
-- "$@" \
`
if [ $? != 0 ] ; then usage 1 ; fi
eval set -- "$TEMP"

#echo "$TEMP"
###################################
# default options
opt_configure=0
opt_submodule_test=0
opt_rpc_dnx=0
opt_rpc_xgs=0
debug=echo

while true; do
  case "$1" in
    -h | --help )      usage 0 ;;
    -c | --configure)  opt_configure=1; shift ;;
    -s | --submodule_add)  opt_submodule_test=1; shift ;;
         --rpc-dnx )       opt_rpc_dnx=1; shift ;;
         --rpc-xgs )       opt_rpc_xgs=1; shift ;;
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


#
# Synatx: _add_patch module_path patch_file_name
#
_add_patch()
{
    echo "_add_patch $2 to $1"
    pushd $1  &>/dev/null
    git apply $2
    popd &>/dev/null
}

apply_patch_files() {
    echo "Apply patch files"
    # https://github.com/sonic-net/sonic-swss/pull/3269
    # [POC] verify route performance issue #3269
    # _add_patch src/sonic-swss ../../fix34kroute.patch
    # _add_patch src/sonic-utilities ../../nokia_route_check.patch
}

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
  git remote remove $2 &>/dev/null || true
  git remote add -f $2 $3
  git cherry-pick --keep-redundant-commits -x $4 || ( git cherry-pick --abort && false )
  popd &>/dev/null
}

_submodule_revert() {
    echo "$prog: submodule change in \"$1\" revert \"$2\""
    pushd $1 &>/dev/null
    if [ -n "${CI_JOB_ID}" ]; then
      # pipeline builds
      git config user.email "${GITLAB_USER_EMAIL}"
      git config user.name "${GITLAB_USER_LOGIN}"
    fi
    git revert $2 --no-edit
    popd &>/dev/null
}

submodule_prs () {
  echo ""
  # Use this to cherry-pick PRs that have not yet been merged to github.
  # args for submodule cherry-picks here: <directory> <user> <https-url> <commit-id>
  _submodule_add src/sonic-swss-common mlok https://github.com/mlok-nokia/sonic-swss-common.git ed4137c7a2cb
  # _submodule_add src/sonic-swss-common mlok https://github.com/mlok-nokia/sonic-swss-common.git ed4137c7a2cb
  # _submodule_add src/sonic-sairedis/SAI saksarav-nokia https://github.com/saksarav-nokia/SAI.git 063fbd4
  # PR To fix the issue: show_techsupport & saidump errors during testbed testing by replacing redis-rdb-tool with rdb-cli
  # https://github.com/sonic-net/sonic-buildimage/pull/19268
  # https://github.com/sonic-net/sonic-sairedis/pull/1391
  _submodule_add src/sonic-sairedis Junhong https://github.com/JunhongMao/sonic-sairedis.git 4b32eaf
  _submodule_add src/sonic-sairedis Junhong https://github.com/JunhongMao/sonic-sairedis.git fb2185c
  _submodule_add ./ Junhong https://github.com/JunhongMao/sonic-buildimage.git 69c914f
  # PR https://github.com/sonic-net/sonic-swss/pull/3247 - fabric link monitor fix
  _submodule_add src/sonic-swss jfeng-arista https://github.com/jfeng-arista/sonic-swss.git 04d5b15^..82bd7cd


  # https://github.com/sonic-net/sonic-platform-daemons/pull/573
  _submodule_add src/sonic-platform-daemons mlok-nokia https://github.com/mlok-nokia/sonic-platform-daemons.git 70caabb
  
  # https://github.com/sonic-net/sonic-utilities/pull/3676  -- show_tech with BERT info
  _submodule_add src/sonic-utilities mlok-nokia https://github.com/mlok-nokia/sonic-utilities.git 9b9e3b3^..8349ac1
   
  # Revert SAI counter enhancement that breaks SAI 11.X (remove when updated to SAI 12.x)
  _submodule_revert src/sonic-sairedis/SAI f23185d

  # https://github.com/sonic-net/sonic-swss/pull/3329  <-- 34k routes
  _submodule_add src/sonic-swss upstream https://github.com/sonic-net/sonic-swss.git d006374
  # apply patches
  apply_patch_files
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

build_rpc_dnx () {
  submodule_prs
  make SONIC_BUILD_JOBS=2 ENABLE_SYNCD_RPC=y target/docker-syncd-brcm-dnx-rpc.gz
}

build_rpc_xgs () {
  submodule_prs
  make SONIC_BUILD_JOBS=2 ENABLE_SYNCD_RPC=y target/docker-syncd-brcm-rpc.gz
}

if [ $opt_submodule_test -eq 1 ]; then
  echo "Test submodule_prs"
  submodule_prs
  exit $?
fi

configure
rc=$?
if [ "$CI_JOB_STAGE" = "configure" -o $opt_configure -eq 1 ]; then
	exit $rc
fi

if [ $opt_rpc_dnx -eq 1 ]; then
  build_rpc_dnx
  exit $?
fi

if [ $opt_rpc_xgs -eq 1 ]; then
  build_rpc_xgs
  exit $?
fi

build
