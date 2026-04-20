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
      --rpc-mrvl        ; build mrvl (arm64) syncd rpc image only
      --features        ; apply features function
      --platform=[marvell-prestera | nokia-vs | vs | aspeed]        ; build mrvl or nokia-vs for CO (arm64 build machine only)
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
-l help,configure,rpc-dnx,rpc-xgs,rpc-mrvl,submodule,features,platform: \
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
opt_features=0
opt_rpc_dnx=0
opt_rpc_xgs=0
opt_rpc_mrvl=0
opt_platform=marvell-prestera
debug=echo

while true; do
  case "$1" in
    -h | --help )      usage 0 ;;
    -c | --configure)  opt_configure=1; shift ;;
    -s | --submodule_add)  opt_submodule_test=1; shift ;;
         --rpc-dnx )       opt_rpc_dnx=1; shift ;;
         --rpc-xgs )       opt_rpc_xgs=1; shift ;;
         --rpc-mrvl )      opt_rpc_mrvl=1; shift ;;
         --features)       opt_features=1; shift ;;
         --platform)       opt_platform=$2; shift ;;
    --        )        shift;  break ;;
    *         )        break ;;
  esac
done

case $opt_platform in
  marvell-prestera|nokia-vs|vs|aspeed)
    ;;
  *)
    echo "Invalid platform: $opt_platform"
    usage 1
    ;;
esac

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
    if [ -e "$2.done" ]; then
        echo "Patch $2 has been done. Skip!"
    else
        git apply $2
        touch $2.done
    fi
    popd &>/dev/null
}

apply_patch_files() {
    echo "Apply patch files"
    # https://github.com/sonic-net/sonic-swss/pull/3269
    # [POC] verify route performance issue #3269
    # _add_patch src/sonic-swss ../../fix34kroute.patch
    # _add_patch src/sonic-utilities ../../utils_pr4012.patch

    # Increase timeout for route_check to 500s
    _add_patch src/sonic-utilities ../../route_check_timeout.patch
    
    # _add_patch src/sonic-swss ../../media_type.patch
    
    # 2/23/2026 commented the following patch fir rebase build
    # _add_patch src/sonic-swss ../../everflow_mirror_swss.patch

    #if [ "${opt_platform}" = "aspeed" ]; then
        # Add sonic-db-gnmi/gnoi in order to support remote access to redis db
        #_add_patch src/sonic-swss-common ../../sonic-db-gnmi.patch

        # Add sonic-bmcutil for BMC CLI
        #_add_patch src/sonic-utilities ../../sonic-bmcutil.patch
    #fi

    # IMPORTANT: When add any patch, please mention the PR number.
    
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

  ############################################################################################################################
  #
  # IMPORTANT:  Please include the PR number info of the cherry-pick PR
  #   Example:
  #     # https://github.com/sonic-net/sonic-platform-daemons/pull/573
  #     _submodule_add src/sonic-platform-daemons mlok-nokia https://github.com/mlok-nokia/sonic-platform-daemons.git 70caabb
  #
  ############################################################################################################################
  #https://github.com/sonic-net/sonic-swss/pull/3660
  _submodule_add src/sonic-swss bala_swss https://github.com/balanokia/sonic-swss a42395a

  #https://github.com/sonic-net/sonic-swss/pull/4001
  _submodule_add src/sonic-swss bala_swss https://github.com/balanokia/sonic-swss 9fa297a

  #https://github.com/sonic-net/sonic-swss/pull/3377
  _submodule_add src/sonic-swss ossobv-swss https://github.com/ossobv/sonic-swss 52ed10e
  
  #tmp c1
  _submodule_add src/sonic-sairedis hehuang-nokia https://github.com/hehuang-nokia/sonic-sairedis.git 2e8392a

  #sonic-bmc aspeed
  #https://github.com/sonic-net/sonic-linux-kernel/pull/553
  _submodule_add src/sonic-linux-kernel nats-nokia https://github.com/nats-nokia/sonic-linux-kernel.git 5c1ba16
  
  # apply patches
  apply_patch_files

}

features()
{
  if [ $opt_features -ne 0 ]; then
    echo "applying features"
    echo INCLUDE_STP=y >> rules/config
    # patch code and/or export env variables that alter build
  fi
}

make init

# echo "Build time patching Nokia sysObjectID"
# ./NokiaSnmpBuildTimePatch.sh

configure()
{
  case $mach in
    x86_64)
      if [ "${opt_platform}" = "vs" ]; then
        make PLATFORM=vs configure
      else
        make PLATFORM=broadcom configure
      fi
      ;;
    armhf)
      make PLATFORM=marvell-prestera PLATFORM_ARCH=armhf configure
      ;;
    aarch64)
      make PLATFORM=${opt_platform} PLATFORM_ARCH=arm64 configure
      ;;
  esac
}

build()
{
  case $mach in
    x86_64)
      submodule_prs
      features
      if [ "${opt_platform}" = "vs" ]; then
        make target/sonic-vs.img.gz || make target/sonic-vs.img.gz
      else
        make target/sonic-broadcom.bin || \
        ( rm -f target/*.bin && make target/sonic-broadcom.bin )
      fi
      ;;
    armhf)
      submodule_prs
      features
      make target/sonic-marvell-prestera-armhf.bin || \
        ( rm -f target/*.bin && make target/sonic-marvell-prestera-armhf.bin )
      ;;
    aarch64)
      submodule_prs
      features
      make target/sonic-${opt_platform}-arm64.bin || \
        ( rm -f target/*.bin && make target/sonic-${opt_platform}-arm64.bin )
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

build_rpc_mrvl () {
  submodule_prs
  make SONIC_BUILD_JOBS=2 ENABLE_SYNCD_RPC=y target/docker-syncd-mrvl-prestera-rpc.gz
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

if [ $opt_rpc_mrvl -eq 1 ]; then
  build_rpc_mrvl
  exit $?
fi

build
