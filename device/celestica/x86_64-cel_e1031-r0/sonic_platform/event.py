try:
    import time
    import select
    from .common import Common
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError(repr(e) + " - required module not found")


class SfpEvent:
    ''' Listen to insert/remove sfp events '''

    SFP_NUM_START = 49
    DELAY = 0.05
    INT_PATH = '/sys/devices/platform/e1031.smc/SFP/modabs_int'
    GPIO_SUS7 = '/sys/devices/platform/hlx-ich.0/sci_int_gpio_sus7'

    def __init__(self, sfp_list):
        self._api_common = Common()
        self._sfp_list = sfp_list
        self._logger = Logger()

        # clear interrupt
        self._api_common.read_one_line_file(self.INT_PATH)

    def get_sfp_event(self, timeout):
        epoll = select.epoll()
        port_dict = {}
        timeout_sec = timeout/1000

        try:
            # We get notified when there is an SCI interrupt from GPIO SUS7
            fd = open(self.GPIO_SUS7, "r")
            fd.read()

            epoll.register(fd.fileno(), select.EPOLLIN & select.EPOLLET)
            events = epoll.poll(timeout=timeout_sec if timeout != 0 else -1)
            if events:
                # Read the QSFP ABS interrupt & status registers
                port_changes = self._api_common.read_one_line_file(
                    self.INT_PATH)
                changes = int(port_changes, 16)
                for sfp in self._sfp_list:
                    if sfp.port_num < self.SFP_NUM_START:
                        continue

                    change = (changes >> sfp.port_num-self.SFP_NUM_START) & 1
                    if change == 1:
                        time.sleep(self.DELAY)
                        port_status = sfp.get_presence()
                        port_dict[str(sfp.port_num)
                                  ] = '1' if port_status else '0'

                return port_dict
        except Exception as e:
            self._logger.log_error("Failed to detect SfpEvent - " + repr(e))
            return False

        finally:
            fd.close()
            epoll.close()

        return False
