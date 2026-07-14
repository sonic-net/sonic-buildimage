#!/usr/bin/env python

from swsssdk import SonicV2Connector

REDIS_HOSTIP = "127.0.0.1"
REBOOT_TYPE_KEXEC_FILE = "/proc/cmdline"
# The following SONIC_BOOT_TYPEs come from the warm/fast reboot script which is in sonic-utilities
# Because the system can be rebooted from some old versions, we have to take all possible BOOT options into consideration.
# On 201803, 201807 we have
# BOOT_OPTIONS="$(echo $KERNEL_OPTIONS | sed -e 's/\s*linux\s*/BOOT_IMAGE=/') fast-reboot"
# On 201811 and later we have
# BOOT_OPTIONS="$(echo $KERNEL_OPTIONS | sed -e 's/\s*linux\s*/BOOT_IMAGE=/') SONIC_BOOT_TYPE=${BOOT_TYPE_ARG}" where BOOT_TYPE_ARG can be warm, fastfast or fast
# To extract the commom part of them, we should have the following PATTERN
REBOOT_TYPE_KEXEC_PATTERN_FASTFAST = ".*SONIC_BOOT_TYPE=(fastfast).*"
REBOOT_TYPE_KEXEC_PATTERN_FAST = ".*SONIC_BOOT_TYPE=(fast|fast-reboot).*"

def getKernelBootType()
{
    """
    get reboot flag from state DB
    """
    state_db = SonicV2Connector(host=REDIS_HOSTIP)
    state_db.connect(state_db.STATE_DB, False)
    TABLE_NAME_SEPARATOR = '|'
    warm_prefix = 'WARM_RESTART_ENABLE_TABLE' + TABLE_NAME_SEPARATOR
    fast_prefix = 'FAST_REBOOT' + TABLE_NAME_SEPARATOR
    _warm_hash = '{}{}'.format(warm_prefix, system)
    _fast_hash = '{}{}'.format(fast_prefix, system

    if os.path.isfile(REBOOT_TYPE_KEXEC_FILE):
        with open(REBOOT_TYPE_KEXEC_FILE, "r") as cause_file:
            cause_file_kexec = cause_file.readline()
        m = re.search(REBOOT_TYPE_KEXEC_PATTERN_FASTFAST, cause_file_kexec)
        if m and m.group(1):
            reboot_type='fastfast'

    elif state_db.get(state_db.STATE_DB, _warm_hash, "enable") == "true":
        reboot_type='warm'
    elif state_db.get(state_db.STATE_DB, _cold_hash, "enable") == "true":
        reboot_type='fast'
    else:
        reboot_type = 'cold'

    state_db.close(state_db.STATE_DB)
    return reboot_type

def main():
    """
    Usage: %(scriptName)s command object
    command:
        get_kernel_boot_type        : get boot type i.e. warm, fastfast or fast|fast-reboot
    """

    if len(sys.argv)<2:
        print main.__doc__

    for arg in sys.argv[1:]:
        if arg == 'get_kernel_boot_type':
            getKernelBootType()
        else:
            print main.__doc__

if __name__ == '__main__':
    main()
