# A stub vendor sonic_platform.platform module, loaded by platform_led_test.rs.
import atexit
import signal
import threading
import time

class Chassis:
    led = None
    initialized = False
    shutdown = False

    def __init__(self):
        # As a vendor chassis watches for shutdown: on the main thread only, chained to the handler before it.
        if threading.current_thread() is threading.main_thread():
            previous = signal.getsignal(signal.SIGTERM)

            def on_term(signum, frame):
                Chassis.shutdown = True
                if callable(previous):
                    previous(signum, frame)

            signal.signal(signal.SIGTERM, on_term)

    def get_name(self):
        return "stub"

    def set_status_led(self, color):
        Chassis.led = color
        return True

    def get_status_led(self):
        return Chassis.led

    def initizalize_system_led(self):
        Chassis.initialized = True
        return True

class BareChassis:
    def get_name(self):
        return "bare"

# A vendor chassis that raises while it is built.
class FailsToBuild:
    def __init__(self):
        raise RuntimeError("chassis init failed")

# The other kinds of LED methods in the vendors' chassis classes.
class Vendor:
    led = None
    calls = 0

class FallsThrough:
    def set_status_led(self, color):
        Vendor.led = color

    def get_status_led(self):
        return None

    def initizalize_system_led(self):
        pass

class AnswersFalse:
    def set_status_led(self, color):
        return False

    def get_status_led(self):
        return False

    def initizalize_system_led(self):
        return False

class Unsupported:
    def set_status_led(self, color):
        raise NotImplementedError

    def get_status_led(self):
        raise NotImplementedError

class Raises:
    def set_status_led(self, color):
        Vendor.calls += 1
        raise OSError("LED write failed")

    def get_status_led(self):
        Vendor.calls += 1
        raise OSError("LED read failed")

class SlowOnce:
    def set_status_led(self, color):
        Vendor.calls += 1
        if Vendor.calls == 1:
            time.sleep(1)
        Vendor.led = color
        return True

    def get_status_led(self):
        return Vendor.led

class Platform:
    chassis = Chassis

    def get_chassis(self):
        return Platform.chassis()

exited = False

def _exit():
    global exited
    exited = True

atexit.register(_exit)
