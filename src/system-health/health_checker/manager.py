from .config import Config
from .health_checker import HealthChecker
from .service_checker import ServiceChecker
from .hardware_checker import HardwareChecker
from .user_defined_checker import UserDefinedChecker
from . import utils


class HealthCheckerManager(object):
    """
    Manage all system health checkers and system health configuration.
    """
    def __init__(self):
        self._checkers = []
        self.config = Config()
        self.initialize()

    def initialize(self):
        """
        Initialize the manager. Create service checker and hardware checker by default.
        :return:
        """
        self._checkers.append(ServiceChecker())
        self._checkers.append(HardwareChecker())

    def check(self, chassis):
        """
        Load new configuration if any and perform the system health check for all existing checkers.
        :param chassis: A chassis object.
        :return: A dictionary that contains the status for all objects that was checked.
        """
        HealthChecker.summary = HealthChecker.STATUS_OK
        stats = {}
        self.config.load_config()

        for checker in self._checkers:
            self._do_check(checker, stats)

        if self.config.user_defined_checkers:
            for udc in self.config.user_defined_checkers:
                checker = UserDefinedChecker(udc)
                self._do_check(checker, stats)

        self._set_system_led(chassis)
        return stats

    def _do_check(self, checker, stats):
        """
        Do check for a particular checker and collect the check statistic.
        :param checker: A checker object.
        :param stats: Check statistic.
        :return:
        """
        try:
            checker.check(self.config)
            category = checker.get_category()
            info = checker.get_info()
            if category not in stats:
                stats[category] = info
            else:
                stats[category].update(info)
        except Exception as e:
            HealthChecker.summary = HealthChecker.STATUS_NOT_OK
            error_msg = 'Failed to perform health check for {} due to exception - {}'.format(checker, repr(e))
            entry = {str(checker): {
                HealthChecker.INFO_FIELD_OBJECT_STATUS: HealthChecker.STATUS_NOT_OK,
                HealthChecker.INFO_FIELD_OBJECT_MSG: error_msg,
                HealthChecker.INFO_FIELD_OBJECT_TYPE: "Internal"
            }}
            if 'Internal' not in stats:
                stats['Internal'] = entry
            else:
                stats['Internal'].update(entry)

    def _set_system_led(self, chassis):
        try:
            chassis.set_status_led(self._get_led_target_color())
        except NotImplementedError:
            print('chassis.set_status_led is not implemented')
        except Exception as e:
            print('Failed to set system led due to - {}'.format(repr(e)))

    def _get_led_target_color(self):
        """Get target LED color according to health status and system uptime

        Returns:
            str: LED color
        """
        if HealthChecker.summary == HealthChecker.STATUS_OK:
            return self.config.get_led_color('normal')
        else:
            uptime = utils.get_uptime()
            return self.config.get_led_color('booting') if uptime < self.config.get_bootup_timeout() else self.config.get_led_color('fault')
