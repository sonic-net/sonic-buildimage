try:
    import time
except ImportError as e:
    raise ImportError(repr(e) + " - required module not found")


class SfpEvent:
    ''' Listen to insert/remove sfp events '''

    def __init__(self, sfp_list):
        self._sfp_list = sfp_list

    sfp_change_event_data = {'valid': 0, 'last': 0, 'present': 0}

    def get_sfp_event(self, timeout=2000):
        now = time.time()
        port_dict = {}
        change_dict = {}
        change_dict['sfp'] = port_dict

        if timeout < 1000:
            timeout = 1000
        timeout = timeout / float(1000)  # Convert to secs

        if now < (self.sfp_change_event_data['last'] + timeout) and self.sfp_change_event_data['valid']:
            return True, change_dict

        bitmap = 0
        for sfp in self._sfp_list:
            modpres = sfp.get_presence()
            i = sfp.port_num-1
            if modpres:
                bitmap = bitmap | (1 << i)

        changed_ports = self.sfp_change_event_data['present'] ^ bitmap
        if changed_ports:
            for sfp in self._sfp_list:
                i = sfp.port_num-1
                if (changed_ports & (1 << i)):
                    if (bitmap & (1 << i)) == 0:
                        port_dict[i+1] = '0'
                    else:
                        port_dict[i+1] = '1'

            # Update the cache dict
            self.sfp_change_event_data['present'] = bitmap
            self.sfp_change_event_data['last'] = now
            self.sfp_change_event_data['valid'] = 1
            return True, change_dict
        else:
            return True, change_dict
