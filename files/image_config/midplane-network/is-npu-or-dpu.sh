#!/bin/bash

OPTIND=1         # Reset in case getopts has been used previously in the shell.

CHECK_ONLY_FOR_DPU=0
CHECK_ONLY_FOR_NPU=0

while getopts "dn" opt; do
  case "$opt" in
    d)  CHECK_ONLY_FOR_DPU=1
      ;;
    n)  CHECK_ONLY_FOR_NPU=1
      ;;
  esac
done

PLATFORM=$(grep onie_platform /host/machine.conf | cut -d= -f 2)

if [[ -z $PLATFORM ]]; then
    PLATFORM=$(grep aboot_platform /host/machine.conf | cut -d= -f 2)
fi

HAS_DPU=0
HAS_NPU=0

if [[ -f /usr/share/sonic/device/${PLATFORM}/platform.json ]]; then
    if jq -e '.DPUS' /usr/share/sonic/device/${PLATFORM}/platform.json >/dev/null; then
        HAS_NPU=1
    fi

    if jq -e '.DPU' /usr/share/sonic/device/${PLATFORM}/platform.json >/dev/null; then
        HAS_DPU=1
    fi
fi

if [[ ${CHECK_ONLY_FOR_DPU} -eq 0 ]] && [[ ${CHECK_ONLY_FOR_NPU} -eq 0 ]]; then
    if [[ ${HAS_DPU} -eq 1 ]] || [[ ${HAS_NPU} -eq 1 ]]; then
        exit 0
    fi
elif [[ ${CHECK_ONLY_FOR_DPU} -eq 1 ]]; then
    if [[ ${HAS_DPU} -eq 1 ]]; then
        exit 0
    fi
else
    if [[ ${HAS_NPU} -eq 1 ]]; then
        exit 0
    fi
fi

exit 1
