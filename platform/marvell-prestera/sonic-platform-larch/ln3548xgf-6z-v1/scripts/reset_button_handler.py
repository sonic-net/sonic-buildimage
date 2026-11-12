#!/usr/bin/env python3
"""
Reset button handler for ln3548xgf-6z-v1.

The button is wired to the board CPLD, not to a CPU GPIO. Left alone the CPLD
owns it: it resets the board itself after a >= 5 s press, and emits a one-clock
global_reset pulse when a short press is released, which wedges the board until
it is power-cycled.

This daemon takes ownership instead. It masks the CPLD's reset-button reset
(Clear_reg) at start, polls the button over I2C, times the press in software and
acts on release.

The press bands, and the SysLed cue that says "you can let go now":

    < 2 s                nothing
    2 s ..  4 s          reboot           - SysLed blinks green twice at 2 s
    > 4 s .. < 10 s      nothing
    10 s .. < 15 s       reset-factory    - SysLed blinks green three times at 10 s
    >= 15 s              the press is abandoned; releasing does nothing

The blinks fire while the button is still held, at the moment the band opens, so
the operator holds until the LED answers instead of counting seconds.

The mask is dropped again (--unmask, wired to ExecStopPost) only when an operator
deliberately stops the daemon, so the hardware path is there whenever somebody
has taken SONiC's handling away on purpose. It is deliberately NOT dropped when
the system is going down, nor when the daemon is merely being restarted - see
--unmask for why handing the button back in those two cases wedges the board.
"""

import argparse
import logging
import logging.handlers
import os
import socket
import subprocess
import sys
import time

# ----------------------------- Configuration -----------------------------

# CPLD lives on the same bus the rest of the platform package uses
# (sonic_platform/fan.py: FAN_I2C_BUS, sonic_platform/led.py: LED_I2CBUS).
CPLD_I2C_BUS = 7

# This daemon's own systemd unit, as list-jobs names it.
UNIT = "reset_button.service"

# One-byte I2C slaves - no register offset, read/written with reg=None.
CPLD_STATE_ADDR = 0x63     # bit 0 = Rst_btn_state (1 = up, 0 = pressed)
CPLD_RST_CTRL_ADDR = 0x61  # bit 0 = Sw_set_rst (DANGER), bit 1 = Clear_reg

RST_BTN_BIT = 0
SW_SET_RST_BIT = 0
CLEAR_REG_BIT = 1

# Press duration bands in seconds, measured from press to release.
REBOOT_MIN = 2         # >= 2 s and <= 4 s  -> reboot
REBOOT_MAX = 4
FACTORY_MIN = 10       # >= 10 s and < 15 s -> reset-factory
ABANDON_AFTER = 15     # past this the press is void whatever happens next

# Number of consecutive polling cycles to confirm the button is up again.
CONFIRMATION_CYCLES = 3

# Polling intervals in seconds. The idle poll runs for the life of the box; the
# fast poll only runs while the button is down and is what times the press.
#
# The idle poll sets how late a press can be noticed, and that error comes
# straight off the measured duration. The reboot band is only 2 s wide, so it has
# to stay well under a second - at 1 s a genuine 2.5 s press could measure 1.5 s
# and do nothing. It is affordable now for the reason the CPU problem was
# affordable to fix: with python3-smbus in the image a poll is an ioctl, not a
# fork of sudo + i2cget, so the frequency was never what cost 5.5% of a core.
POLL_INTERVAL_DEFAULT = 0.25
POLL_INTERVAL_FAST = 0.1

# SysLed is touched only while a press is in progress, to say "the band you want
# is open, let go now". Idle costs nothing.
SYSLED_BLINK_COLOR = "green"
REBOOT_BLINKS = 2
FACTORY_BLINKS = 3
SYSLED_BLINK_ON = 0.25
SYSLED_BLINK_OFF = 0.25

# Where the press trace goes so it outlives the press.
#
# A factory reset deletes /var/log, so the daemon's record of the press that
# caused it is destroyed by that very press - and that is precisely the press
# anyone investigating afterwards comes looking for. reset-factory spares
# /var/log/systemlog (its PERMLOG) and writes its own line there; the trace of
# the button that invoked it belongs beside it, in the same format.
PERMLOG = "/var/log/systemlog"

# ---------------------------------------------------------------------------

log = logging.getLogger("reset_button")

# I2C access: prefer the platform package, fall back to the i2c-tools CLI the
# same way sonic_platform/i2c.py does when smbus is missing.
try:
    from sonic_platform.i2c import i2c_read_byte_data, i2c_write_byte_data
except ImportError:
    def i2c_read_byte_data(i2c_bus, i2c_addr, reg=None):
        cmd = ["i2cget", "-y", str(i2c_bus), hex(i2c_addr)]
        if reg is not None:
            cmd.append(hex(reg))
        out = subprocess.run(cmd, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE, text=True, check=True)
        return int(out.stdout.strip(), 16)

    def i2c_write_byte_data(i2c_bus, i2c_addr, reg, data):
        cmd = ["i2cset", "-y", str(i2c_bus), hex(i2c_addr)]
        if reg is not None:
            cmd.append(hex(reg))
        cmd.append(hex(data))
        subprocess.run(cmd, stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE, text=True, check=True)


def permlog(message):
    """
    Append one line to the log that survives a factory reset.

    Best effort from end to end: a log that cannot be written must never stop
    the button from working, so a failure here is reported to syslog and
    otherwise ignored.
    """
    try:
        stamp = time.strftime("%Y/%m/%d %H:%M:%S")
        host = socket.gethostname().split(".")[0]
        with open(PERMLOG, "a") as f:
            f.write("%s %s reset_button: %s\n" % (stamp, host, message))
    except Exception as e:
        log.warning("cannot write %s: %s", PERMLOG, e)


def _system_is_shutting_down():
    """
    True while systemd is taking the system down.

    ExecStopPost fires both when an operator stops the unit and when the whole
    system reboots, and those two want opposite things from the mask.

    On error this answers False, which keeps the previous behaviour rather than
    silently holding the mask: a mask left set while nobody is polling disables
    the physical recovery button, and that is the worse of the two mistakes to
    make blind.
    """
    try:
        out = subprocess.run(["systemctl", "is-system-running"],
                             stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                             text=True)
        return out.stdout.strip() == "stopping"
    except Exception as e:
        log.warning("cannot tell whether the system is stopping: %s", e)
        return False


def _stopping_for_restart():
    """
    The reason this stop is one the daemon is coming straight back from, or None.

    Two of those. A crash that Restart=always is about to undo: systemd sets
    SERVICE_RESULT for ExecStopPost, "success" for a stop somebody asked for and
    a failure reason otherwise. And an explicit `systemctl restart`, which sets
    SERVICE_RESULT=success exactly as a plain stop does - the two are told apart
    by the job systemd still has queued for the unit, whose type reads "restart"
    for the first and "stop" for the second.

    Handing the button back for either gap is a window in which a press wedges
    the board, for no gain. Measured on #31969: 0.70 s of it across a restart.

    On error this answers None, unmasking, for the reason given in
    _system_is_shutting_down.
    """
    result = os.environ.get("SERVICE_RESULT", "success")
    if result != "success":
        return "SERVICE_RESULT=%s" % result

    try:
        out = subprocess.run(["systemctl", "list-jobs", "--no-legend",
                              "--no-pager", UNIT],
                             stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                             text=True)
        for line in out.stdout.splitlines():
            fields = line.split()
            # JOB  UNIT  TYPE  STATE
            if len(fields) >= 3 and fields[1] == UNIT and fields[2] != "stop":
                return "a %s job is queued for %s" % (fields[2], UNIT)
    except Exception as e:
        log.warning("cannot tell whether this stop is a restart: %s", e)

    return None


def _write_rst_ctrl(clear_reg):
    """
    Write the reset control register. Sw_set_rst (bit 0) resets the board the
    instant it is set, so it is guarded here rather than trusted to callers.
    """
    value = (1 << CLEAR_REG_BIT) if clear_reg else 0
    if value & (1 << SW_SET_RST_BIT):
        raise AssertionError("refusing to set Sw_set_rst")
    i2c_write_byte_data(CPLD_I2C_BUS, CPLD_RST_CTRL_ADDR, None, value)


def mask_cpld_reset():
    """Take the button away from the CPLD."""
    _write_rst_ctrl(True)


def unmask_cpld_reset():
    """Give the button back to the CPLD."""
    _write_rst_ctrl(False)


def button_pressed():
    """
    Returns:
        True  - button is held
        False - button is up
        None  - the read failed; the caller skips the cycle
    """
    try:
        value = i2c_read_byte_data(CPLD_I2C_BUS, CPLD_STATE_ADDR)
    except Exception as e:
        log.warning("cannot read button state: %s", e)
        return None

    # The i2c-tools fallback in sonic_platform/i2c.py returns the command's exit
    # status on failure instead of raising, so a negative value is not data.
    if value is None or value < 0:
        return None

    return not (value >> RST_BTN_BIT) & 1


def _open_sysled():
    """Return a SysLed handle, or None if it cannot be driven on this image."""
    try:
        from sonic_platform.led import (I2CLed, I2CRegisterBitField,
                                        LED_I2CBUS, LED_I2CADDR,
                                        SYSLED_MASK, SYSLED_STATES)
    except ImportError:
        return None

    try:
        return I2CLed(I2CRegisterBitField(LED_I2CBUS, LED_I2CADDR, None,
                                          SYSLED_MASK, SYSLED_STATES))
    except Exception as e:
        log.warning("cannot open SysLed: %s", e)
        return None


def blink_sysled(count):
    """
    Blink SysLed `count` times, then put it back to the colour it was showing.

    Best effort from end to end: the button must keep working on an image where
    the platform package is missing or the LED register cannot be reached.

    The button is not sampled while this runs (about half a second per blink).
    That is deliberate and harmless: a blink fires at the instant a band opens,
    so a release during it is attributed to the end of the blink, still inside
    the band it announced - see the band table in the module docstring.
    """
    led = _open_sysled()
    if led is None:
        return

    try:
        previous = led.get_state()
        # get_state() answers "invalid" for a bit pattern it does not know; do
        # not hand that back to set_state(), it would raise.
        if previous not in ("off", "green", "red", "amber"):
            previous = "green"

        for _ in range(count):
            led.set_state("off")
            time.sleep(SYSLED_BLINK_OFF)
            led.set_state(SYSLED_BLINK_COLOR)
            time.sleep(SYSLED_BLINK_ON)

        led.set_state(previous)
    except Exception as e:
        log.warning("cannot blink SysLed: %s", e)


def handle_reboot(duration):
    log.info("press of %.2f s - rebooting", duration)
    permlog("press of %.2f s - rebooting" % duration)
    subprocess.run(["reboot"], check=False)


def handle_factory_reset(duration):
    log.info("press of %.2f s - running reset-factory", duration)
    permlog("press of %.2f s - running reset-factory" % duration)
    subprocess.run(["reset-factory"], check=False)


def act_on_release(duration, abandoned):
    """Run the action a completed press earns, if it earned one."""
    if abandoned:
        log.info("press of %.2f s was abandoned at %d s - ignored",
                 duration, ABANDON_AFTER)
        permlog("press of %.2f s was abandoned at %d s - ignored"
                % (duration, ABANDON_AFTER))
    elif REBOOT_MIN <= duration <= REBOOT_MAX:
        handle_reboot(duration)
    elif FACTORY_MIN <= duration < ABANDON_AFTER:
        handle_factory_reset(duration)
    else:
        log.info("press of %.2f s is in no band - ignored", duration)
        permlog("press of %.2f s is in no band - ignored" % duration)


def poll_loop():
    # State definitions
    STATE_UP = 0
    STATE_PRESSED = 1
    STATE_CONFIRMING_UP = 2

    state = STATE_UP
    poll_interval = POLL_INTERVAL_DEFAULT
    confirmation_counter = 0
    press_start_time = None
    release_time = None
    # Cues already given for this press, and whether it has been written off.
    reboot_signalled = False
    factory_signalled = False
    abandoned = False

    while True:
        pressed = button_pressed()

        if pressed is None:
            # Read failed. Hold the current state - synthesising a release here
            # could fire reset-factory on a button nobody let go of.
            time.sleep(poll_interval)
            continue

        if state == STATE_UP:
            if pressed:
                state = STATE_PRESSED
                poll_interval = POLL_INTERVAL_FAST
                press_start_time = time.time()
                reboot_signalled = factory_signalled = abandoned = False
                log.info("button pressed")
                permlog("button pressed")
        elif state == STATE_PRESSED:
            if pressed:
                confirmation_counter = 0
                held = time.time() - press_start_time
                # One cue per band, at the moment the band opens. Checked
                # newest band first so a poll delayed by a blink cannot skip one.
                if not abandoned and held >= ABANDON_AFTER:
                    abandoned = True
                    log.info("held past %d s - press abandoned, release will do"
                             " nothing", ABANDON_AFTER)
                    permlog("held past %d s - press abandoned" % ABANDON_AFTER)
                elif not factory_signalled and held >= FACTORY_MIN:
                    factory_signalled = True
                    log.info("held %d s - release now for reset-factory",
                             FACTORY_MIN)
                    permlog("held %d s - reset-factory band open, SysLed blinking %d times"
                            % (FACTORY_MIN, FACTORY_BLINKS))
                    blink_sysled(FACTORY_BLINKS)
                elif not reboot_signalled and held >= REBOOT_MIN:
                    reboot_signalled = True
                    log.info("held %d s - release now to reboot", REBOOT_MIN)
                    permlog("held %d s - reboot band open, SysLed blinking %d times"
                            % (REBOOT_MIN, REBOOT_BLINKS))
                    blink_sysled(REBOOT_BLINKS)
            else:
                # Remember when the button was FIRST seen up: the cycles below
                # only confirm it, and counting them into the press would push a
                # 4 s hold out of the reboot band.
                state = STATE_CONFIRMING_UP
                confirmation_counter = 1
                release_time = time.time()
        elif state == STATE_CONFIRMING_UP:
            if pressed:
                # Contact bounce, not a release.
                state = STATE_PRESSED
                confirmation_counter = 0
                release_time = None
            else:
                confirmation_counter += 1
                if confirmation_counter >= CONFIRMATION_CYCLES:
                    duration = release_time - press_start_time
                    was_abandoned = abandoned
                    state = STATE_UP
                    poll_interval = POLL_INTERVAL_DEFAULT
                    confirmation_counter = 0
                    press_start_time = None
                    release_time = None
                    act_on_release(duration, was_abandoned)

        time.sleep(poll_interval)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unmask", action="store_true",
                        help="hand the button back to the CPLD and exit")
    parser.add_argument("--blink", type=int, metavar="N",
                        help="blink SysLed N times and exit, without touching the"
                             " button or the CPLD mask - isolates the LED write"
                             " from the press when checking what the panel does")
    args = parser.parse_args()

    handler = logging.handlers.SysLogHandler(address="/dev/log")
    handler.setFormatter(logging.Formatter("reset_button: %(message)s"))
    log.addHandler(handler)
    log.addHandler(logging.StreamHandler())
    log.setLevel(logging.INFO)

    if args.unmask:
        # Handing the button back to the CPLD is right when an operator has
        # deliberately stopped the daemon, and wrong in the two cases below -
        # in both of them the CPLD would take a press on the pure-hardware
        # path, which emits the one-clock global_reset that wedges the board
        # until somebody power-cycles it. That is the very failure this ticket
        # exists to remove, and the likeliest press of all is the operator's
        # own: a 2-4 s press runs reboot, the unit stops early in the shutdown,
        # and the button is still under their thumb.
        if _system_is_shutting_down():
            log.info("system is stopping - leaving the CPLD reset-button path"
                     " masked; power-off clears it in hardware")
            permlog("system stopping - CPLD reset-button path left masked")
            return 0
        restart = _stopping_for_restart()
        if restart:
            log.info("daemon is coming straight back (%s) - leaving the CPLD"
                     " reset-button path masked", restart)
            return 0

        try:
            unmask_cpld_reset()
        except Exception as e:
            log.error("cannot unmask CPLD reset: %s", e)
            return 1
        log.info("CPLD reset-button path restored")
        permlog("daemon stopped by operator - CPLD reset-button path restored")
        return 0

    if args.blink is not None:
        # Deliberately before mask_cpld_reset(): this is a panel check, and it
        # must not take the button away from the CPLD on its way out.
        blink_sysled(args.blink)
        return 0

    try:
        mask_cpld_reset()
    except Exception as e:
        # Polling unmasked is worse than not polling at all: the user would get
        # the short-press hang and no software handling either. Let systemd retry.
        log.error("cannot mask CPLD reset: %s", e)
        return 1
    log.info("CPLD reset-button path masked - button is owned by this daemon")

    try:
        poll_loop()
    except KeyboardInterrupt:
        pass
    return 0


if __name__ == "__main__":
    sys.exit(main())
