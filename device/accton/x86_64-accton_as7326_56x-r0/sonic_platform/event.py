try:
    import time
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError(repr(e) + " - required module not found")

POLL_INTERVAL_IN_SEC = 1

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
            i=sfp.port_num-1
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
                i=sfp.port_num-1
                if (changed_ports & (1 << i)):
                    if (bitmap & (1 << i)) == 0:
                        port_dict[i+1] = '0'
                    else:
                        port_dict[i+1] = '1'


            # Update the cache dict
            self._sfp_change_event_data['present'] = bitmap
            return True, change_dict
        else:
            return True, change_dict
