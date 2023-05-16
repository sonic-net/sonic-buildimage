#
# Copyright (c) 2020-2021 NVIDIA CORPORATION & AFFILIATES.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
import os
import time

from sonic_py_common.logger import Logger
from . import utils
from . import device_data

logger = Logger()


class Led(object):
    STATUS_LED_COLOR_GREEN = 'green'
    STATUS_LED_COLOR_RED = 'red'
    STATUS_LED_COLOR_ORANGE = 'orange'
    STATUS_LED_COLOR_BLUE = 'blue'
    STATUS_LED_COLOR_OFF = 'off'
    STATUS_LED_COLOR_GREEN_BLINK = 'green_blink'
    STATUS_LED_COLOR_RED_BLINK = 'red_blink'
    STATUS_LED_COLOR_ORANGE_BLINK = 'orange_blink'
    STATUS_LED_COLOR_BLUE_BLINK = 'blue_blink'

    LED_ON = '255'
    LED_OFF = '0'
    LED_BLINK = '50'

    SIMILAR_COLORS = {
        'red': ('amber', 'orange'),
        'amber': ('red', 'orange'),
        'orange': ('red', 'amber')
    }

    PRIMARY_COLORS = {
        'red': 'red',
        'amber': 'red',
        'orange': 'red',
        'green': 'green',
        'blue': 'blue'
    }

    LED_PATH = "/var/run/hw-management/led/"

    def __init__(self):
        self.supported_colors = set()
        self.supported_blinks = set()

    def _get_actual_color(self, color):
        # Different platform has different LED capability, this capability should be
        # transparent for upper layer. So, here is the logic to help find actual color
        # if the given color is not supported.
        if color not in self.supported_colors:
            return self._get_similar_color(color)
        return color

    def _get_similar_color(self, color):
        # If a given color is not supported, we try to find a similar color from
        # canditates
        similar_colors = self.SIMILAR_COLORS.get(color)
        if similar_colors:
            for actual_color in similar_colors:
                if actual_color in self.supported_colors:
                    return actual_color
        return None

    def _get_primary_color(self, color):
        # For backward compatible, we don't return the actual color here.
        # We always return "green"(indicate a good status) or "red"(indicate a bad status) 
        # which are the "primary" colors.
        return self.PRIMARY_COLORS.get(color, color)

    def _get_actual_blink_color(self, blink_color):
        # Different platform has different LED capability, this capability should be
        # transparent for upper layer. So, here is the logic to help find actual blink color
        # if the given blink color is not supported.
        if blink_color not in self.supported_blinks:
            return self._get_similar_blink_color(blink_color)
        return blink_color

    def _get_similar_blink_color(self, color):
        # If a given blink color is not supported, we try to find a similar blink color from
        # canditates
        similar_colors = self.SIMILAR_COLORS.get(color)
        if similar_colors:
            for actual_color in similar_colors:
                if actual_color in self.supported_blinks:
                    return actual_color
        return None

    def set_status(self, color):
        self.get_capability()
        if not self.supported_colors:
            if not device_data.DeviceDataManager.is_simx_platform():
                logger.log_error(f'Failed to get LED capability for {self._led_id} LED')
            return False

        status = False
        try:
            self._stop_blink()
            blink_pos = color.find('_blink')
            if blink_pos != -1:
                return self._set_status_blink(color[0:blink_pos])

            if color != Led.STATUS_LED_COLOR_OFF:
                actual_color = self._get_actual_color(color)
                if not actual_color:
                    logger.log_error(f'Set LED to color {color} is not supported')
                    return False

                utils.write_file(self.get_led_path(actual_color), Led.LED_ON)
                status = True
            else:
                for color in self.supported_colors:
                    utils.write_file(self.get_led_path(color), Led.LED_OFF)

                status = True
        except (ValueError, IOError):
            status = False

        return status

    def _set_status_blink(self, color):
        actual_color = self._get_actual_blink_color(color)
        if not actual_color:
            logger.log_error(f'Set LED to color {color}_blink is not supported')
            return False

        self._trigger_blink(self.get_led_trigger(actual_color))
        return self._set_led_blink_status(actual_color)

    def _stop_blink(self):
        try:
            for color in self.supported_colors:
                self._untrigger_blink(self.get_led_trigger(color))
        except Exception as e:
            return

    def _trigger_blink(self, blink_trigger_file):
        utils.write_file(blink_trigger_file, 'timer')

    def _untrigger_blink(self, blink_trigger_file):
        utils.write_file(blink_trigger_file, 'none')

    def _set_led_blink_status(self, actual_color):
        delay_on_file = self.get_led_delay_on_path(actual_color)
        delay_off_file = self.get_led_delay_off_path(actual_color)
        if not self._wait_files_ready((delay_on_file, delay_off_file)):
            return False

        utils.write_file(delay_on_file, Led.LED_BLINK)
        utils.write_file(delay_off_file, Led.LED_BLINK)
        return True

    def _wait_files_ready(self, file_list):
        """delay_off and delay_on sysfs will be available only if _trigger_blink is called. And once
           _trigger_blink is called, driver might need time to prepare delay_off and delay_on. So,
           need wait a few seconds here to make sure the sysfs is ready

        Args:
            file_list (list of str): files to be checked
        """
        wait_time = 5.0
        initial_sleep = 0.01
        while wait_time > 0:
            if all([os.path.exists(x) for x in file_list]):
                return True
            time.sleep(initial_sleep)
            wait_time -= initial_sleep
            initial_sleep = initial_sleep * 2

        return False

    def get_status(self):
        self.get_capability()
        if not self.supported_colors:
            if not device_data.DeviceDataManager.is_simx_platform():
                logger.log_error(f'Failed to get LED capability for {self._led_id} LED')
            return Led.STATUS_LED_COLOR_OFF

        try:
            blink_status = self._get_blink_status()
            if blink_status is not None:
                return blink_status

            actual_color = None
            for color in self.supported_colors:
                if utils.read_str_from_file(self.get_led_path(color)) != Led.LED_OFF:
                    actual_color = color
                    break

            if actual_color is not None:
                return self._get_primary_color(actual_color)
        except (ValueError, IOError) as e:
            raise RuntimeError("Failed to read led status due to {}".format(repr(e)))

        return Led.STATUS_LED_COLOR_OFF

    def _get_blink_status(self):
        try:
            for color in self.supported_colors:
                if self._is_led_blinking(color):
                    return f'{color}_blink'
        except Exception as e:
            return None

        return None

    def _is_led_blinking(self, color):
        delay_on = utils.read_str_from_file(self.get_led_delay_on_path(color), default=Led.LED_OFF, log_func=None)
        delay_off = utils.read_str_from_file(self.get_led_delay_off_path(color), default=Led.LED_OFF, log_func=None)
        return delay_on != Led.LED_OFF and delay_off != Led.LED_OFF

    def get_capability(self):
        caps = utils.read_str_from_file(self.get_led_cap_path())
        for capability in caps.split():
            if capability == 'none':
                continue

            pos = capability.find('_blink')
            if pos != -1:
                self.supported_blinks.add(capability[0:pos])
            else:
                self.supported_colors.add(capability)

    def get_led_path(self, color):
        return os.path.join(Led.LED_PATH, f'led_{self._led_id}_{color}')

    def get_led_trigger(self, color):
        return os.path.join(Led.LED_PATH, f'led_{self._led_id}_{color}_trigger')

    def get_led_delay_off_path(self, color):
        return os.path.join(Led.LED_PATH, f'led_{self._led_id}_{color}_delay_off')

    def get_led_delay_on_path(self, color):
        return os.path.join(Led.LED_PATH, f'led_{self._led_id}_{color}_delay_on')

    def get_led_cap_path(self):
        return os.path.join(Led.LED_PATH, f'led_{self._led_id}_capability')


class FanLed(Led):
    def __init__(self, index):
        super().__init__()
        if index is not None:
            self._led_id = 'fan{}'.format(index)
        else:
            self._led_id = 'fan'


class PsuLed(Led):
    def __init__(self, index):
        super().__init__()
        if index is not None:
            self._led_id = 'psu{}'.format(index)
        else:
            self._led_id = 'psu'


class SystemLed(Led):
    def __init__(self):
        super().__init__()
        self._led_id = 'status'


class SystemUidLed(Led):
    def __init__(self):
        super().__init__()
        self._led_id = 'uid'


class SharedLed(object):
    # for shared LED, blink is not supported for now. Currently, only PSU and fan LED
    # might be shared LED, and there is no requirement to set PSU/fan LED to blink status.
    LED_PRIORITY = {
        Led.STATUS_LED_COLOR_RED: 0,
        Led.STATUS_LED_COLOR_GREEN: 1
    }

    def __init__(self, led):
        self._led = led
        self._virtual_leds = []

    def add_virtual_leds(self, led):
        self._virtual_leds.append(led)

    def update_status_led(self):
        target_color = Led.STATUS_LED_COLOR_GREEN
        for virtual_led in self._virtual_leds:
            try:
                if SharedLed.LED_PRIORITY[virtual_led.get_led_color()] < SharedLed.LED_PRIORITY[target_color]:
                    target_color = virtual_led.get_led_color()
            except KeyError:
                return False
        return self._led.set_status(target_color)

    def get_status(self):
        return self._led.get_status()


class ComponentFaultyIndicator(object):
    def __init__(self, shared_led):
        self._color = Led.STATUS_LED_COLOR_GREEN
        self._shared_led = shared_led
        self._shared_led.add_virtual_leds(self)

    def set_status(self, color):
        current_color = self._color
        self._color = color
        if self._shared_led.update_status_led():
            return True
        else:
            self._color = current_color
            return False

    def get_led_color(self):
        return self._color

    def get_status(self):
        return self._shared_led.get_status()
