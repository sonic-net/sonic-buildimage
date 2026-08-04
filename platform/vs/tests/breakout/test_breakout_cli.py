from swsscommon import swsscommon
import time
import os
import json
import ast
import pytest
import collections

@pytest.mark.usefixtures('dpb_setup_fixture')
class TestBreakoutCli(object):
    def setup_db(self, dvs):
        self.cdb = swsscommon.DBConnector(4, dvs.redis_sock, 0)

    def read_Json(self, dvs):
        test_dir = os.path.dirname(os.path.realpath(__file__))
        sample_output_file = os.path.join(test_dir, 'sample_output', 'sample_new_port_config.json')
        with open(sample_output_file, 'rb') as fh:
            fh_data = json.load(fh)

        if not fh_data:
            return False
        expected = ast.literal_eval(json.dumps(fh_data))
        return expected

    def breakout(self, dvs, interface, brkout_mode):
        (exitcode, result) = dvs.runcmd("config interface breakout {} {} -y".format(interface, brkout_mode))

        if result.strip("\n")[0] == "[ERROR] Breakout feature is not available without platform.json file" :
            pytest.skip("**** This test is not needed ****")
        root_dir = os.path.dirname('/')
        (exitcode, output_dict) = dvs.runcmd("jq '.' new_port_config.json")
        if output_dict is None:
            raise Exception("Breakout output cant be None")

        output_dict = ast.literal_eval(output_dict.strip())
        return output_dict

    # Check Initial Brakout Mode
    def test_InitialBreakoutMode(self, dvs, testlog):
        self.setup_db(dvs)

        output_dict = {}
        brkoutTbl = swsscommon.Table(self.cdb, "BREAKOUT_CFG")
        brkout_entries = brkoutTbl.getKeys()
        assert len(brkout_entries) == 32

        for key in brkout_entries:
            (status, fvs) = brkoutTbl.get(key)
            assert status

            brkout_mode = fvs[0][1]
            output_dict[key] = brkout_mode
        output = collections.OrderedDict(sorted(output_dict.items(), key=lambda t: t[0]))
        expected_dict = \
                {'Ethernet8': '1x100G[40G]', 'Ethernet0': '1x100G[40G]', 'Ethernet4': '1x100G[40G]', \
                'Ethernet108': '1x100G[40G]', 'Ethernet100': '1x100G[40G]', 'Ethernet104': '1x100G[40G]', \
                'Ethernet68': '1x100G[40G]', 'Ethernet96': '1x100G[40G]', 'Ethernet124': '1x100G[40G]', \
                'Ethernet92': '1x100G[40G]', 'Ethernet120': '1x100G[40G]', 'Ethernet52': '1x100G[40G]', \
                'Ethernet56': '1x100G[40G]', 'Ethernet76': '1x100G[40G]', 'Ethernet72': '1x100G[40G]', \
                'Ethernet32': '1x100G[40G]', 'Ethernet16': '1x100G[40G]', 'Ethernet36': '1x100G[40G]', \
                'Ethernet12': '1x100G[40G]', 'Ethernet28': '1x100G[40G]', 'Ethernet88': '1x100G[40G]', \
                'Ethernet116': '1x100G[40G]', 'Ethernet80': '1x100G[40G]', 'Ethernet112': '1x100G[40G]', \
                'Ethernet84': '1x100G[40G]', 'Ethernet48': '1x100G[40G]', 'Ethernet44': '1x100G[40G]', \
                'Ethernet40': '1x100G[40G]', 'Ethernet64': '1x100G[40G]', 'Ethernet60': '1x100G[40G]', \
                'Ethernet20': '1x100G[40G]', 'Ethernet24': '1x100G[40G]'}
        expected = collections.OrderedDict(sorted(expected_dict.items(), key=lambda t: t[0]))
        assert output == expected

    # Breakout Cli Test Mode
    def test_breakout_modes(self, dvs):
        expected = self.read_Json(dvs)
        assert expected

        print("**** Breakout Cli test Starts ****")
        output_dict = self.breakout(dvs, 'Ethernet0', '2x50G')
        expected_dict = expected["Ethernet0_2x50G"]
        assert output_dict == expected_dict, "output: {} != expected: {}".format(output_dict, expected_dict)
        print("**** 1X100G --> 2x50G passed ****")

        output_dict = self.breakout(dvs, 'Ethernet4', '4x25G[10G]')
        expected_dict = expected["Ethernet4_4x25G"]
        assert output_dict == expected_dict, "output: {} != expected: {}".format(output_dict, expected_dict)
        print("**** 1X100G --> 4x25G[10G] passed ****")

        output_dict = self.breakout(dvs, 'Ethernet8', '2x25G(2)+1x50G(2)')
        expected_dict = expected["Ethernet8_2x25G_1x50G"]
        assert output_dict == expected_dict, "output: {} != expected: {}".format(output_dict, expected_dict)
        print("**** 1X100G --> 2x25G(2)+1x50G(2) passed ****")

        output_dict = self.breakout(dvs, 'Ethernet12', '1x50G(2)+2x25G(2)')
        expected_dict = expected["Ethernet12_1x50G_2x25G"]
        assert output_dict == expected_dict, "output: {} != expected: {}".format(output_dict, expected_dict)
        print("**** 1X100G --> 1x50G(2)+2x25G(2) passed ****")

        # TODOFIX: remove comments once #4442 PR got merged and
        # yang model for DEVICE_METADATA becomes available.
        # As below test cases are dependent on DEVICE_METADATA to go
        # from a non-default breakout mode to a different breakout mode.
        """
        output_dict = self.breakout(dvs, 'Ethernet0', '1x100G[40G]')
        expected_dict = expected["Ethernet0_1x100G"]
        assert output_dict == expected_dict
        print("**** 2x50G --> 1x100G[40G] passed ****")

        output_dict = self.breakout(dvs, 'Ethernet0', '4x25G[10G]')
        expected_dict = expected["Ethernet0_4x25G"]
        assert output_dict == expected_dict
        print("**** 1X100G --> 4x25G[10G] passed ****")

        output_dict = self.breakout(dvs, 'Ethernet0', '1x100G[40G]')
        expected_dict = expected["Ethernet0_1x100G"]
        assert output_dict == expected_dict
        print("**** 4x25G[10G] --> 1x100G[40G] passed ****")

        output_dict = self.breakout(dvs, 'Ethernet4', '2x50G')
        print("**** 1X100G --> 2x50G mode change ****")

        output_dict = self.breakout(dvs, 'Ethernet4', '4x25G[10G]')
        expected_dict = expected["Ethernet4_4x25G"]
        assert output_dict == expected_dict
        print("**** 2X50G --> 4x25G[10G] passed ****")

        output_dict = self.breakout(dvs, 'Ethernet4', '2x50G')
        expected_dict = expected["Ethernet4_2x50G"]
        assert output_dict == expected_dict
        print("**** 4x25G[10G] --> 2X50G passed ****")

        output_dict = self.breakout(dvs, 'Ethernet4', '1x100G[40G]')
        print("**** 2x50G  -- > 1X100G mode change ****")

        output_dict = self.breakout(dvs, 'Ethernet0', '2x25G(2)+1x50G(2)')
        expected_dict = expected["Ethernet0_2x25G_1x50G"]
        assert output_dict == expected_dict
        print("**** 1x100G[40G] --> 2x25G(2)+1x50G(2) passed ****")

        output_dict = self.breakout(dvs, 'Ethernet0', '1x100G[40G]')
        expected_dict = expected["Ethernet0_1x100G"]
        assert output_dict == expected_dict
        print("**** 2x25G(2)+1x50G(2) --> 1x100G[40G] passed ****")

        output_dict = self.breakout(dvs, 'Ethernet0', '1x50G(2)+2x25G(2)')
        expected_dict = expected["Ethernet0_1x50G_2x25G"]
        assert output_dict == expected_dict
        print("**** 1x100G[40G] --> 1x50G(2)+2x25G(2)  passed ****")

        output_dict = self.breakout(dvs, 'Ethernet0', '1x100G[40G]')
        expected_dict = expected["Ethernet0_1x100G"]
        assert output_dict == expected_dict
        print("**** 1x50G(2)+2x25G(2) --> 1x100G[40G] passed ****")

        output_dict = self.breakout(dvs, 'Ethernet8', '2x50G')
        print("**** 1x100G[40G] --> 2x50G  mode change ****")

        output_dict = self.breakout(dvs, 'Ethernet8', '1x50G(2)+2x25G(2)')
        expected_dict = expected["Ethernet8_1x50G_2x25G"]
        assert output_dict == expected_dict
        print("**** 2x50G --> 2x25G(2)+1x50G(2)  passed ****")

        output_dict = self.breakout(dvs, 'Ethernet8', '2x25G(2)+1x50G(2)')
        expected_dict = expected["Ethernet8_2x25G_1x50G"]
        assert output_dict == expected_dict
        print("**** 1x50G(2)+2x25G(2) --> 2x25G(2)+1x50G(2)  passed ****")

        output_dict = self.breakout(dvs, 'Ethernet8', '1x100G[40G]')
        expected_dict = expected["Ethernet8_1x100G"]
        assert output_dict == expected_dict
        print("**** 2x25G(2)+1x50G(2)  --> 1x100G[40G]  passed ****")
        """
