#!/usr/bin/env python3

import subprocess

from sonic_py_common import logger
from swsscommon import swsscommon

log = logger.Logger('mark_dhcp_packet')


class MarkDhcpPacket(object):
    """
    Class used to configure dhcp packet mark in ebtables
    """

    def __init__(self):
        self.config_db_connector = None
        self.state_db_connector = None

    @property
    def config_db(self):
        """
        Returns config DB connector.
        Initializes the connector during the first call
        """
        if self.config_db_connector is None:
            self.config_db_connector = swsscommon.ConfigDBConnector()
            self.config_db_connector.connect()

        return self.config_db_connector

    @property
    def state_db(self):
        """
        Returns the state DB connector.
        Initializes the connector during the first call
        """
        if self.state_db_connector is None:
            self.state_db_connector = swsscommon.SonicV2Connector(
                                                    host='127.0.0.1'
                                                )
            self.state_db_connector.connect(self.state_db_connector.STATE_DB)

        return self.state_db_connector

    @property
    def is_dualtor(self):
        """
        Checks if script is running on a dual ToR system
        """
        localhost_key = self.config_db.get_keys('DEVICE_METADATA')[0]
        metadata = self.config_db.get_entry('DEVICE_METADATA', localhost_key)

        return 'subtype' in metadata and \
               'dualtor' in metadata['subtype'].lower()

    def get_mux_intfs(self):
        """
        Returns a list of mux cable interfaces
        """
        mux_cables = self.config_db.get_table('MUX_CABLE')
        mux_intfs = [intf for intf in mux_cables]

        return mux_intfs

    def generate_mark_from_index(self, index):
        '''
        type: string, format: hexadecimal
        Example: 0x67001, 0x67002, ...
        '''
        intf_mark = "0x67" + str(index).zfill(3)

        return intf_mark

    def run_command(self, cmd):
        subprocess.call(cmd)
        log.log_info("run command: {}".format(cmd))

    def clear_dhcp_packet_marks(self):
        '''
        Flush the INPUT chain in ebtables upon restart
        '''
        self.run_command(["sudo", "ebtables", "-F", "INPUT"])

    def apply_mark_in_ebtables(self, intf, mark):
        self.run_command(["sudo", "ebtables", "-A", "INPUT", "-i", intf, "-j", "mark", "--mark-set", mark])

    def update_mark_in_state_db(self, intf, mark):
        self.state_db.set(
            self.state_db.STATE_DB,
            'DHCP_PACKET_MARK|' + intf,
            'mark',
            mark
        )

    def apply_marks(self):
        """
        Writes dhcp packet marks in ebtables
        """
        if not self.is_dualtor:
            return

        self.clear_dhcp_packet_marks()

        for (index, intf) in enumerate(self.get_mux_intfs(), 1):
            mark = self.generate_mark_from_index(index)
            self.apply_mark_in_ebtables(intf, mark)
            self.update_mark_in_state_db(intf, mark)

        log.log_info("Finish marking dhcp packets in ebtables.")


if __name__ == '__main__':
    mark_dhcp_packet = MarkDhcpPacket()
    mark_dhcp_packet.apply_marks()
