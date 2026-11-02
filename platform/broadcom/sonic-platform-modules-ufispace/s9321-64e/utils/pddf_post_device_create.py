#!/usr/bin/env python3
import sys

def set_xcvr_lpmode_status(platform_chassis, lpmode_status):
    """ 
    Execute Hardware LPMode STATUS for all SFPs.
    
    Args:
        platform_chassis: The platform chassis instance.
        lpmode_status (bool): True to enable hardware LPMode, False to disable.
    """
    try:
        sfp_list = platform_chassis.get_all_sfps() or []
    except Exception as e:
        print(f"[ERR] Failed to get SFP list: {e}", file=sys.stderr)
        return False

    if not sfp_list:
        print(f"[WARN] No SFPs list returned from this platform.")
        return True

    for sfp in sfp_list:
        if sfp is None:
            continue
        try:
            sfp.set_lpmode_hardware(lpmode_status)

        except Exception as e:
            pass

    status_log = "ON" if lpmode_status else "OFF"
    print(f"[INFO] Successfully set hardware LPMode {status_log}.")
    return True


def main():
    try:
        import sonic_platform.platform
        platform_chassis = sonic_platform.platform.Platform().get_chassis()
        if not platform_chassis:
            raise RuntimeError("Platform chassis instance is None")
    except Exception as e:
        print(f"[ERR] Chassis init failed: {e}", file=sys.stderr)
        sys.exit(1)

    set_xcvr_lpmode_status(platform_chassis, lpmode_status=False)

    sys.exit(0)

main()
