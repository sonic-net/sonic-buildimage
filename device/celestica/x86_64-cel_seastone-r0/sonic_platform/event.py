try:
    import select
    from .helper import APIHelper
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError(repr(e) + " - required module not found")


class SfpEvent:
    ''' Listen to insert/remove sfp events '''

    QSFP_MODPRS_IRQ = '/sys/devices/platform/dx010_cpld/qsfp_modprs_irq'
    GPIO_SUS6 = "/sys/devices/platform/slx-ich.0/sci_int_gpio_sus6"

    def __init__(self, sfp_list):
        self._api_helper = APIHelper()
        self._sfp_list = sfp_list
        self._logger = Logger()

    def get_sfp_event(self, timeout):
        epoll = select.epoll()
        port_dict = {}
        timeout_sec = timeout/1000

        try:
            # We get notified when there is an SCI interrupt from GPIO SUS6
            fd = open(self.GPIO_SUS6, "r")
            fd.read()

            epoll.register(fd.fileno(), select.EPOLLIN & select.EPOLLET)
            events = epoll.poll(timeout=timeout_sec if timeout != 0 else -1)
            if events:
                # Read the QSFP ABS interrupt & status registers
                port_changes = self._api_helper.read_one_line_file(
                    self.QSFP_MODPRS_IRQ)
                changes = int(port_changes, 16)
                for sfp in self._sfp_list:
                    change = (changes >> sfp.port_num-1) & 1
                    if change == 1:
                        port_dict[str(sfp.port_num)] = str(
                            int(sfp.get_presence()))

                return port_dict
        except Exception as e:
            self._logger.log_error("Failed to detect SfpEvent - " + repr(e))
            return False

        finally:
            fd.close()
            epoll.close()

        return False
