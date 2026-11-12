#!/usr/bin/env python3
import argparse
import time

# Import platform LED controller
from sonic_platform.led import LedController


def blink(controller, name, color, duration=20, period=1.0):
    """Blink a specific LED with given period and duration."""
    end_time = time.time() + duration
    half = period / 2.0
    while time.time() < end_time:
        controller.set_led(name, color)
        time.sleep(half)
        controller.set_led(name, LedController.STATUS_LED_COLOR_OFF)
        time.sleep(half)


def main():
    parser = argparse.ArgumentParser(description="Locator LED control tool for SONiC")
    parser.add_argument(
        "action",
        choices=["on_green", "on_red", "blink_green", "blink_red", "off"],
        help="LED action to perform"
    )
    args = parser.parse_args()

    # Initialize LED controller (I2C bus/addr defined in led.py)
    led = LedController()

    LOC = "loc"

    if args.action == "on_green":
        led.set_led(LOC, LedController.STATUS_LED_COLOR_GREEN)

    elif args.action == "on_red":
        led.set_led(LOC, LedController.STATUS_LED_COLOR_AMBER)

    elif args.action == "blink_green":
        blink(led, LOC, LedController.STATUS_LED_COLOR_GREEN)

    elif args.action == "blink_red":
        blink(led, LOC, LedController.STATUS_LED_COLOR_AMBER)

    elif args.action == "off":
        led.set_led(LOC, LedController.STATUS_LED_COLOR_OFF)


if __name__ == "__main__":
    main()
