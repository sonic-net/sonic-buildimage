# -*- coding: utf-8 -*-

try:
    from sonic_platform.regutil import Reg
    from sonic_platform.logger import logger
except ImportError:
    raise ImportError(str(e) + "- required module not found")

class Rotor:
    def __init__(self, config):
        if config is not None and isinstance(config, dict):
            self.__reg_speed_getter = Reg(config.get("speed_getter"))
            self.__reg_speed_setter = Reg(config.get("speed_setter"))
            self.__speed_max = config.get("speed_max")
        else:
            raise ValueError("init rotor Error: {}".format(config))

    def get_speed(self):
        try:
            return int(self.__reg_speed_getter.decode())
        except Exception as e:
            logger.error(str(e))

        return 0

    def set_speed(self, speed):
        try:
            return self.__reg_speed_setter.encode(speed)
        except Exception as e:
            logger.error(str(e))

        return False

    def get_speed_percentage(self):
        try:
            speed = self.get_speed()
            return (100 * speed) / self.__speed_max
        except Exception as e:
            logger.error(str(e))

        return 0
