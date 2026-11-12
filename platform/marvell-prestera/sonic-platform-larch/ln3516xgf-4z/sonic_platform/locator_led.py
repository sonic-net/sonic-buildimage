#!/usr/bin/env python3

import argparse
import sys

from sonic_platform.platform import Platform


def main():
    parser = argparse.ArgumentParser(description="Control the chassis locator LED")
    parser.add_argument("action", choices=("on", "off"))
    args = parser.parse_args()

    chassis = Platform().get_chassis()
    color = "green" if args.action == "on" else "off"
    if not chassis.set_uid_led(color):
        print("Failed to set locator LED {}".format(args.action), file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
