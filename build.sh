#!/bin/bash
set -e

source nokia-env

mach=$(uname -m)

if (echo ${CI_RUNNER_TAGS} | grep armhf ); then
  mach=armhf
fi

make init

configure()
{
  case $mach in
    x86_64)
      make PLATFORM=broadcom configure
      ;;
    armhf)
      make PLATFORM=marvell-armhf PLATFORM_ARCH=armhf configure
      ;;
    aarch64)
      make PLATFORM=marvell-arm64 PLATFORM_ARCH=arm64 configure
      ;;
  esac
}

build()
{
  case $mach in
    x86_64)
      make SONIC_BUILD_JOBS=2 target/sonic-broadcom.bin || \
        ( rm -f target/*.bin && make target/sonic-broadcom.bin )
      ;;
    armhf)
      make SONIC_BUILD_JOBS=2 target/sonic-marvell-armhf.bin || \
        ( rm -f target/*.bin && make target/sonic-marvell-armhf.bin )
      ;;
    aarch64)
      make SONIC_BUILD_JOBS=2 target/sonic-marvell-arm64.bin || \
        ( rm -f target/*.bin && make target/sonic-marvell-arm64.bin )
      ;;
  esac

}

configure
rc=$?
if [ "$CI_JOB_STAGE" = "configure" ]; then
	exit $rc
fi

build
