try:
    import select
    import time
    from .helper import APIHelper
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError(repr(e) + " - required module not found")


QSFP_MODPRS_IRQ = '/sys/devices/platform/dx010_cpld/qsfp_modprs_irq'
GPIO_SUS6 = "/sys/devices/platform/slx-ich.0/sci_int_gpio_sus6"
QSFP_EVENT_POLLING_MODE = True


class SfpEvent:
    ''' Listen to insert/remove sfp events '''

    def __init__(self, sfp_list):
        self._api_helper = APIHelper()
        self._sfp_list = sfp_list
        self._logger = Logger()

    sfp_change_event_data = {'valid': 0, 'last': 0, 'present': 0}

    def get_sfp_event(self, timeout):
        port_dict = {}

        if QSFP_EVENT_POLLING_MODE:
            # Using polling mode
            now = time.time()

            if timeout < 1000:
                timeout = 1000
            timeout = timeout / float(1000)  # Convert to secs

            if now < (self.sfp_change_event_data['last'] + timeout) \
                    and self.sfp_change_event_data['valid']:
                return True, port_dict

            bitmap = 0
            for sfp in self._sfp_list:
                modpres = sfp.get_presence()
                index = sfp.get_index()
                if modpres:
                    bitmap = bitmap | (1 << index)

            changed_ports = self.sfp_change_event_data['present'] ^ bitmap
            if changed_ports:
                for sfp in self._sfp_list:
                    index = sfp.get_index()
                    if changed_ports & (1 << index):
                        if (bitmap & (1 << index)) == 0:
                            port_dict[str(index + 1)] = '0'
                        else:
                            port_dict[str(index + 1)] = '1'

                # Update the cache dict
                self.sfp_change_event_data['present'] = bitmap
                self.sfp_change_event_data['last'] = now
                self.sfp_change_event_data['valid'] = 1

            return True, port_dict

        # SFP event from GPIO interrupt pin
        epoll = select.epoll()
        timeout_sec = timeout/1000

        try:
            # We get notified when there is an SCI interrupt from GPIO SUS6
            fd = open(GPIO_SUS6, "r")
            fd.read()

            epoll.register(fd.fileno(), select.EPOLLIN & select.EPOLLET)
            events = epoll.poll(timeout=timeout_sec if timeout != 0 else -1)
            if events:
                # Read the QSFP ABS interrupt & status registers
                port_changes = self._api_helper.read_one_line_file(
                    QSFP_MODPRS_IRQ)
                changes = int(port_changes, 16)
                for sfp in self._sfp_list:
                    index = sfp.get_index()
                    change = (changes >> index) & 1
                    if change == 1:
                        port_dict[str(index + 1)] = str(
                            int(sfp.get_presence()))

                return True, port_dict
        except Exception as e:
            self._logger.log_error("Failed to detect SfpEvent - " + repr(e))
            return False, port_dict

        finally:
            fd.close()
            epoll.close()

        return False, port_dict
