try:
    import time
    from sonic_py_common.logger import Logger
    from .sfp import Sfp
except ImportError as e:
    raise ImportError(repr(e) + " - required module not found")

POLL_INTERVAL_IN_SEC = 1

# SFP errors that will block eeprom accessing
SFP_BLOCKING_ERRORS = [
    Sfp.SFP_ERROR_BIT_I2C_STUCK,
    Sfp.SFP_ERROR_BIT_BAD_EEPROM,
    Sfp.SFP_ERROR_BIT_UNSUPPORTED_CABLE,
    Sfp.SFP_ERROR_BIT_HIGH_TEMP,
    Sfp.SFP_ERROR_BIT_BAD_CABLE
]

class SfpEvent:
    ''' Listen to insert/remove sfp events '''

    def __init__(self, sfp_list):
        self._sfp_list = sfp_list
        self._logger = Logger()
        self._sfp_change_event_data = {'present': 0}

    def get_presence_bitmap(self):
        bitmap = 0
        for sfp in self._sfp_list:
            modpres = sfp.get_presence()
            i=sfp.get_position_in_parent() - 1
            if modpres:
                bitmap = bitmap | (1 << i)
        return bitmap

    def get_sfp_event(self, timeout=2000):
        port_dict = {}
        change_dict = {}
        change_dict['sfp'] = port_dict

        if timeout < 1000:
            cd_ms = 1000
        else:
            cd_ms = timeout

        while cd_ms > 0:
            bitmap = self.get_presence_bitmap()
            changed_ports = self._sfp_change_event_data['present'] ^ bitmap
            if changed_ports != 0:
                break
            time.sleep(POLL_INTERVAL_IN_SEC)
            # timeout=0 means wait for event forever
            if timeout != 0:
                cd_ms = cd_ms - POLL_INTERVAL_IN_SEC * 1000

        if changed_ports != 0:
            for sfp in self._sfp_list:
                i=sfp.get_position_in_parent() - 1
                if (changed_ports & (1 << i)) == 0:
                    continue

                if (bitmap & (1 << i)) == 0:
                    port_dict[i+1] = '0'
                else:
                    sfp_state_bits = self.get_sfp_state_bits(sfp, True)
                    sfp_state_bits = self.check_sfp_blocking_errors(sfp_state_bits)

                    port_dict[i+1] = str(sfp_state_bits)

            # Update the cache dict
            self._sfp_change_event_data['present'] = bitmap
            return True, change_dict
        else:
            return True, change_dict

    def get_sfp_state_bits(self, sfp, present):
        sfp_state_bits = 0

        if present is True:
            sfp_state_bits |= Sfp.SFP_STATUS_BIT_INSERTED
        else:
            return sfp_state_bits

        status = sfp.refresh_optoe_dev_class()
        if status is not True:
            if status is None:
                sfp_state_bits |= Sfp.SFP_ERROR_BIT_I2C_STUCK
                return sfp_state_bits
            else:
                sfp_state_bits |= Sfp.SFP_ERROR_BIT_BAD_EEPROM
                return sfp_state_bits

        status = sfp.validate_eeprom()
        if status is None:
            sfp_state_bits |= Sfp.SFP_ERROR_BIT_I2C_STUCK
            return sfp_state_bits
        elif status is not True:
            sfp_state_bits |= Sfp.SFP_ERROR_BIT_BAD_EEPROM
            return sfp_state_bits

        status = sfp.validate_temperature()
        if status is None:
            sfp_state_bits |= Sfp.SFP_ERROR_BIT_I2C_STUCK
            return sfp_state_bits
        elif status is not True:
            sfp_state_bits |= Sfp.SFP_ERROR_BIT_HIGH_TEMP
            return sfp_state_bits

        return sfp_state_bits

    def check_sfp_blocking_errors(self, sfp_state_bits):
        for i in SFP_BLOCKING_ERRORS:
            if (i & sfp_state_bits) == 0:
                continue
            sfp_state_bits |= Sfp.SFP_ERROR_BIT_BLOCKING

        return sfp_state_bits
