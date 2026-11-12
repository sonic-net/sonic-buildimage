#!/usr/bin/env python3
import argparse
import time

# Import platform LED controller
from sonic_platform.led import LedController


BLINK_DURATION = 20  # seconds


def main():
    parser = argparse.ArgumentParser(description="Locator LED control tool for SONiC")
    parser.add_argument(
        "action",
        choices=["on_green", "on_yellow", "blink_green", "blink_yellow", "off"],
        help="LED action to perform"
    )
    args = parser.parse_args()

    led = LedController()
    LOC = "status2"

    if args.action == "on_green":
        led.set_led(LOC, LedController.STATUS_LED_COLOR_GREEN)

    elif args.action == "on_yellow":
        led.set_led(LOC, LedController.STATUS_LED_COLOR_YELLOW)

    elif args.action == "blink_green":
        led.set_led(LOC, LedController.STATUS_LED_COLOR_GREEN_BLINK)
        time.sleep(BLINK_DURATION)
        led.set_led(LOC, LedController.STATUS_LED_COLOR_OFF)

    elif args.action == "blink_yellow":
        led.set_led(LOC, LedController.STATUS_LED_COLOR_YELLOW_BLINK)
        time.sleep(BLINK_DURATION)
        led.set_led(LOC, LedController.STATUS_LED_COLOR_OFF)

    elif args.action == "off":
        led.set_led(LOC, LedController.STATUS_LED_COLOR_OFF)


if __name__ == "__main__":
    main()
