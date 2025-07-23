try:
    import time
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError(repr(e) + " - required module not found")

class XcvrEvent:
    ''' Listen to insert/remove QSFP/SFP+ events '''

    def __init__(self, sfp_list):
        self._sfp_list = sfp_list
        self._logger = Logger()

    xcvr_change_event_data = {'valid': 0, 'last': 0, 'present': 0}

    def get_xcvr_event(self, timeout):
        port_dict = {}

        # Using polling mode
        now = time.time()

        if timeout < 1000:
            timeout = 1000
        timeout = timeout / float(1000)  # Convert to secs

        if now < (self.xcvr_change_event_data['last'] + timeout) \
                and self.xcvr_change_event_data['valid']:
            return True, port_dict

        bitmap = 0
        for sfp in self._sfp_list:
            modpres = sfp.get_presence()
            index = sfp.port_index - 1
            if modpres:
                bitmap = bitmap | (1 << index)

        changed_ports = self.xcvr_change_event_data['present'] ^ bitmap
        if changed_ports:
            for sfp in self._sfp_list:
                index = sfp.port_index - 1
                if changed_ports & (1 << index):
                    if (bitmap & (1 << index)) == 0:
                        port_dict[str(index + 1)] = '0'
                    else:
                        port_dict[str(index + 1)] = '1'

            # Update the cache dict
            self.xcvr_change_event_data['present'] = bitmap
            self.xcvr_change_event_data['last'] = now
            self.xcvr_change_event_data['valid'] = 1

        return True, port_dict
