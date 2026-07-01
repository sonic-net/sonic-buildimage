#!/usr/bin/env python3


def main():
    try:
        import sonic_platform.platform
        import sonic_platform.chassis
    except ImportError as e:
        raise ImportError(str(e) + "- required module not found")

    chassis = sonic_platform.platform.Platform().get_chassis()
    if chassis is None:
        print "DEBUG chassis was None  "

    base_mac = chassis.get_base_mac()
    print base_mac

    return

if __name__ == '__main__':
    main()
