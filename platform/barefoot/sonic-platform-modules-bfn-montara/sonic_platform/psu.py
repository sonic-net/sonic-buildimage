#!/usr/bin/env python

try:
    import os
    import sys
    import time
    import signal
    import syslog
    import logging
    import threading

    sys.path.append(os.path.dirname(__file__))

    from .platform_thrift_client import thrift_try

    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.thermal import psu_thermals_list_get
    from platform_utils import cancel_on_sigterm
    from sonic_platform.bfn_extensions.psu_sensors import get_psu_metrics
    from sonic_platform.bfn_extensions.psu_sensors import get_metric_value
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

class Psu(PsuBase):
    """Platform-specific PSU class"""

    __lock = threading.Lock()
    __sensors_info = None
    __timestamp = 0

    # When psud gets termination signal it starts processing last cycle.
    # This cycle must be as fast as possible to be able to stop correctly,
    # otherwise it will be killed, so the whole plugin must encounter
    # this signal to process operations based on state, where the
    # state is "termination signal got" and "no termination signal"

    # State is "no termination signal"
    sigterm = False
    sigterm_default_handler = None
    cls_inited = False

    def __init__(self, index):
        PsuBase.__init__(self)
        self.__index = index
        self.__thermals = None
        self.__info = None
        self.__ts = 0
        # STUB IMPLEMENTATION
        self.color = ""

        syslog.syslog(syslog.LOG_INFO, "Created PSU #{} instance".format(self.__index))
        if not Psu.cls_inited:
            Psu.sigterm_default_handler = signal.getsignal(signal.SIGTERM)
            signal.signal(signal.SIGTERM, Psu.signal_handler)
            if Psu.sigterm_default_handler:
                syslog.syslog(syslog.LOG_INFO, "Default SIGTERM handler overridden!!")
            Psu.cls_inited = True

    @classmethod
    def signal_handler(cls, sig, frame):
        if cls.sigterm_default_handler:
            cls.sigterm_default_handler(sig, frame)
        syslog.syslog(syslog.LOG_INFO, "Canceling PSU platform API calls...")
        # Changing state to "termination signal"
        cls.sigterm = True

    @classmethod
    def __sensors_get(cls, cached=True):
        cls.__lock.acquire()
        # Operation may take a few seconds to process, so if state is
        # "termination signal", plugin doesn't perform this operation
        if time.time() > cls.__timestamp + 15 and not Psu.sigterm:
            # Update cache once per 15 seconds
            try:
                cls.__sensors_info = get_psu_metrics()
                cls.__timestamp = time.time()
            except Exception as e:
                logging.warning("Failed to update sensors cache: " + str(e))
        info = cls.__sensors_info
        cls.__lock.release()
        return info

    '''
    Units of returned info object values:
        vin - V
        iout - mA
        vout - V
        pwr_out - mW
        fspeed - RPM
    '''
    def __info_get(self):
        @cancel_on_sigterm
        def psu_info_get(client):
            return client.pltfm_mgr.pltfm_mgr_pwr_supply_info_get(self.__index)

        # Operation may take a few seconds to process, so if state is
        # "termination signal", plugin doesn't perform this operation
        # Update cache once per 2 seconds
        if self.__ts + 2 < time.time() and not Psu.sigterm:
            self.__info = None
            try:
                self.__info = thrift_try(psu_info_get, attempts=1)
            except Exception as e:
                if "Canceling" in str(e):
                    syslog.syslog(syslog.LOG_INFO, "{}".format(e))
            finally:
                self.__ts = time.time()
                return self.__info
        return self.__info

    @cancel_on_sigterm
    def get_metric_value(self, metric_name):
        return get_metric_value(Psu.__sensors_get(), "PSU{} ".format(self.__index) + metric_name)

    @staticmethod
    def get_num_psus():
        """
        Retrieves the number of PSUs available on the device
        :return: An integer, the number of PSUs available on the device
        """
        return 2

    def get_name(self):
        return f"psu-{self.__index}"

    def get_powergood_status(self):
        """
        Retrieves the oprational status of power supply unit (PSU) defined
                by 1-based self.index <self.index>
        :param self.index: An integer, 1-based self.index of the PSU of which to query status
        :return: Boolean, True if PSU is operating properly, False if PSU is faulty
        """
        info = self.__info_get()
        if info is None:
            return False
        return info.ffault == False and info.vout != 0

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        return self.get_metric_value("12V Output Voltage_in1_input")

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        return self.get_metric_value("12V Output Current_curr2_input")

    def get_input_voltage(self):
        """
        Retrieves current PSU voltage input
        Returns:
            A float number, the input voltage in volts,
            e.g. 220
        """
        return self.get_metric_value("Input Voltage_in0_input")

    def get_input_current(self):
        """
        Retrieves the input current draw of the power supply
        Returns:
            A float number, the electric current in amperes, e.g 0.8
        """
        return self.get_metric_value("Input Current_curr1_input")

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        info = self.__info_get()
        return info.pwr_out / 1000 if info else 0

    def get_presence(self):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by 1-based self.index <self.index>
        :param self.index: An integer, 1-based self.index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        @cancel_on_sigterm
        def psu_present_get(client):
            return client.pltfm_mgr.pltfm_mgr_pwr_supply_present_get(self.__index)

        status = False
        if Psu.sigterm:
            return status

        try:
            status = thrift_try(psu_present_get, attempts=1)
        except Exception as e:
            if "Canceling" in str(e):
                syslog.syslog(syslog.LOG_INFO, "{}".format(e))
        finally:
            return status

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED

        Args:
            color: A string representing the color with which to set the
                   PSU status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        # STUB IMPLEMENTATION
        self.color = color
        return True

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        # STUB IMPLEMENTATION
        return self.color

    # DeviceBase iface:
    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        info = self.__info_get()
        return info.serial if info else "N/A"

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        info = self.__info_get()
        return info.model if info else "N/A"

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        info = self.__info_get()
        return info.rev if info else "N/A"

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_powergood_status()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self.__index

    @cancel_on_sigterm
    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        # Operation may take a few seconds to process, so if state is
        # "termination signal", plugin doesn't perform this operation
        return self.get_thermal(0).get_temperature()

    @cancel_on_sigterm
    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU
        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        # Operation may take a few seconds to process, so if state is
        # "termination signal", plugin doesn't perform this operation
        return self.get_thermal(0).get_high_threshold()

    @property
    def _thermal_list(self):
        if self.__thermals is None:
            self.__thermals = psu_thermals_list_get(self.get_name())
        return self.__thermals

    @_thermal_list.setter
    def _thermal_list(self, value):
        pass

def psu_list_get():
    psu_list = []
    for i in range(1, Psu.get_num_psus() + 1):
        psu = Psu(i)
        psu_list.append(psu)
    return psu_list
