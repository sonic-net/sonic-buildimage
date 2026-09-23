"""
GS_timing.py
-create some low-level Arduino-like millis() (milliseconds) and micros()
 (microseconds) timing functions for Python
By Gabriel Staples
http://www.ElectricRCAircraftGuy.com
-click "Contact me" at the top of my website to find my email address
Started: 11 July 2016
Updated: 13 Aug 2016

History (newest on top):
20160813 - v0.2.0 created - added Linux compatibility, using ctypes, so that it's compatible with pre-Python 3.3 (for Python 3.3 or later just use the built-in time functions for Linux, shown here: https://docs.python.org/3/library/time.html)
-ex: time.clock_gettime(time.CLOCK_MONOTONIC_RAW)
20160711 - v0.1.0 created - functions work for Windows *only* (via the QPC timer)

References:
WINDOWS:
-personal (C++ code): GS_PCArduino.h
1) Acquiring high-resolution time stamps (Windows)
   -https://msdn.microsoft.com/en-us/library/windows/desktop/dn553408(v=vs.85).aspx
2) QueryPerformanceCounter function (Windows)
   -https://msdn.microsoft.com/en-us/library/windows/desktop/ms644904(v=vs.85).aspx
3) QueryPerformanceFrequency function (Windows)
   -https://msdn.microsoft.com/en-us/library/windows/desktop/ms644905(v=vs.85).aspx
4) LARGE_INTEGER union (Windows)
   -https://msdn.microsoft.com/en-us/library/windows/desktop/aa383713(v=vs.85).aspx

-*****https://stackoverflow.com/questions/4430227/python-on-win32-how-to-get-
absolute-timing-cpu-cycle-count

LINUX:
-https://stackoverflow.com/questions/1205722/how-do-i-get-monotonic-time-durations-in-python


"""

import ctypes, os
import time, datetime, timeit

# Constants:
VERSION = '0.2.0'

# -------------------------------------------------------------------
# FUNCTIONS:
# -------------------------------------------------------------------
# OS-specific low-level timing functions:
if (os.name == 'nt'):  # for Windows:
    def micros():
        "return a timestamp in microseconds (us)"
        tics = ctypes.c_int64()
        freq = ctypes.c_int64()

        # get ticks on the internal ~2MHz QPC clock
        ctypes.windll.Kernel32.QueryPerformanceCounter(ctypes.byref(tics))
        # get the actual freq. of the internal ~2MHz QPC clock
        ctypes.windll.Kernel32.QueryPerformanceFrequency(ctypes.byref(freq))

        t_us = tics.value * 1e6 / freq.value
        return t_us


    def millis():
        "return a timestamp in milliseconds (ms)"
        tics = ctypes.c_int64()
        freq = ctypes.c_int64()

        # get ticks on the internal ~2MHz QPC clock
        ctypes.windll.Kernel32.QueryPerformanceCounter(ctypes.byref(tics))
        # get the actual freq. of the internal ~2MHz QPC clock
        ctypes.windll.Kernel32.QueryPerformanceFrequency(ctypes.byref(freq))

        t_ms = tics.value * 1e3 / freq.value
        return t_ms

elif (os.name == 'posix'):  # for Linux:

    # Constants:
    CLOCK_MONOTONIC_RAW = 4  # see <linux/time.h> here: https://github.com/torvalds/linux/blob/master/include/uapi/linux/time.h


    # prepare ctype timespec structure of {long, long}
    class timespec(ctypes.Structure):
        _fields_ = \
            [
                ('tv_sec', ctypes.c_long),
                ('tv_nsec', ctypes.c_long)
            ]


    # Configure Python access to the clock_gettime C library, via ctypes:
    # Documentation:
    # -ctypes.CDLL: https://docs.python.org/3.2/library/ctypes.html
    # -librt.so.1 with clock_gettime: https://docs.oracle.com/cd/E36784_01/html/E36873/librt-3lib.html #-
    # -Linux clock_gettime(): http://linux.die.net/man/3/clock_gettime
    librt = ctypes.CDLL('librt.so.1', use_errno=True)
    clock_gettime = librt.clock_gettime
    # specify input arguments and types to the C clock_gettime() function
    # (int clock_ID, timespec* t)
    clock_gettime.argtypes = [ctypes.c_int, ctypes.POINTER(timespec)]


    def monotonic_time():
        "return a timestamp in seconds (sec)"
        t = timespec()
        # (Note that clock_gettime() returns 0 for success, or -1 for failure, in
        # which case errno is set appropriately)
        # -see here: http://linux.die.net/man/3/clock_gettime
        if clock_gettime(CLOCK_MONOTONIC_RAW, ctypes.pointer(t)) != 0:
            # if clock_gettime() returns an error
            errno_ = ctypes.get_errno()
            raise OSError(errno_, os.strerror(errno_))
        return t.tv_sec + t.tv_nsec * 1e-9  # sec


    def micros():
        "return a timestamp in microseconds (us)"
        return monotonic_time() * 1e6  # us


    def millis():
        "return a timestamp in milliseconds (ms)"
        return monotonic_time() * 1e3  # ms


# Other timing functions:
def delay(delay_ms):
    "delay for delay_ms milliseconds (ms)"
    t_start = millis()
    while (millis() - t_start < delay_ms):
        pass  # do nothing
    return


def delayMicroseconds(delay_us):
    "delay for delay_us microseconds (us)"
    t_start = micros()
    while (micros() - t_start < delay_us):
        pass  # do nothing
    return


class Timer:
    """Measure time used."""
    # Ref: https://stackoverflow.com/a/57931660/
    def __init__(self, round_ndigits: int = 0):
        self._round_ndigits = round_ndigits
        self._start_time = timeit.default_timer()

    def __call__(self) -> float:
        return timeit.default_timer() - self._start_time

    def __str__(self) -> str:
        return str(datetime.timedelta(seconds=round(self(), self._round_ndigits)))


def timerdemo():
    # begin timer
    t00 = time.time()
    timer = Timer()

    time.sleep(1)
    print(f"{timer}")
    print(type(timer()),timer())

    time.sleep(5)
    print(f"{timer}")
    print(type(timer()),timer())

    # reset timer
    timer1 = Timer()

    time.sleep(1)
    print(f"{timer}")
    print(type(timer()),timer())
    print(f"{timer1}")
    print(type(timer1()),timer1())

    time.sleep(5)
    print(f"{timer()}")
    print(type(timer()),timer())
    print(f"{timer1}")
    print(type(timer1()),timer1())





# -------------------------------------------------------------------
# EXAMPLES:
# -------------------------------------------------------------------
# Only executute this block of code if running this module directly,
# *not* if importing it
# -see here: http://effbot.org/pyfaq/tutor-what-is-if-name-main-for.htm
if __name__ == "__main__":  # if running this module as a stand-alone program

    # print loop execution time 100 times, using micros()
    tStart = micros()  # us
    for x in range(0, 100):
        tNow = micros()  # us
        dt = tNow - tStart  # us; delta time
        tStart = tNow  # us; update
        print("dt(us) = " + str(dt))

    # print loop execution time 100 times, using millis()
    print("\n")
    tStart = millis()  # ms
    for x in range(0, 100):
        tNow = millis()  # ms
        dt = tNow - tStart  # ms; delta time
        tStart = tNow  # ms; update
        print("dt(ms) = " + str(dt))

    # print a counter once per second, for 5 seconds, using delay
    print("\nstart")
    for i in range(1, 6):
        delay(1000)
        print(i)

    # print a counter once per second, for 5 seconds, using delayMicroseconds
    print("\nstart")
    for i in range(1, 6):
        delayMicroseconds(1000000)
        print(i)

    timerdemo()
