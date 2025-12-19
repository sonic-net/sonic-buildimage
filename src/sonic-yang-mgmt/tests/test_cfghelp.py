import json
import subprocess
import os
from unittest import TestCase

techsupport_table_output="""\

AUTO_TECHSUPPORT
Description: AUTO_TECHSUPPORT part of config_db.json

key - GLOBAL
+-------------------------+----------------------------------------------------+-------------+-----------+-------------+
| Field                   | Description                                        | Mandatory   | Default   | Reference   |
+=========================+====================================================+=============+===========+=============+
| state                   | Knob to make techsupport invocation event-driven   |             |           |             |
|                         | based on core-dump generation                      |             |           |             |
+-------------------------+----------------------------------------------------+-------------+-----------+-------------+
| rate_limit_interval     | Minimum time in seconds between two successive     |             |           |             |
|                         | techsupport invocations. Configure 0 to explicitly |             |           |             |
|                         | disable                                            |             |           |             |
+-------------------------+----------------------------------------------------+-------------+-----------+-------------+
| max_techsupport_limit   | Max Limit in percentage for the cummulative size   |             |           |             |
|                         | of ts dumps. No cleanup is performed if the value  |             |           |             |
|                         | isn't configured or is 0.0                         |             |           |             |
+-------------------------+----------------------------------------------------+-------------+-----------+-------------+
| max_core_limit          | Max Limit in percentage for the cummulative size   |             |           |             |
|                         | of core dumps. No cleanup is performed if the      |             |           |             |
|                         | value isn't congiured or is 0.0                    |             |           |             |
+-------------------------+----------------------------------------------------+-------------+-----------+-------------+
| available_mem_threshold | Memory threshold; 0 to disable techsupport         |             | 10.0      |             |
|                         | invocation on memory usage threshold crossing      |             |           |             |
+-------------------------+----------------------------------------------------+-------------+-----------+-------------+
| min_available_mem       | Minimum Free memory (in MB) that should be         |             | 200       |             |
|                         | available for the techsupport execution to start   |             |           |             |
+-------------------------+----------------------------------------------------+-------------+-----------+-------------+
| since                   | Only collect the logs & core-dumps generated since |             |           |             |
|                         | the time provided. A default value of '2 days ago' |             |           |             |
|                         | is used if this value is not set explicitly or a   |             |           |             |
|                         | non-valid string is provided                       |             |           |             |
+-------------------------+----------------------------------------------------+-------------+-----------+-------------+

"""

techsupport_table_field_output="""\

AUTO_TECHSUPPORT
Description: AUTO_TECHSUPPORT part of config_db.json

key - GLOBAL
+---------+--------------------------------------------------+-------------+-----------+-------------+
| Field   | Description                                      | Mandatory   | Default   | Reference   |
+=========+==================================================+=============+===========+=============+
| state   | Knob to make techsupport invocation event-driven |             |           |             |
|         | based on core-dump generation                    |             |           |             |
+---------+--------------------------------------------------+-------------+-----------+-------------+

"""

vlan_table_field_output="""\

VLAN
Description: VLAN part of config_db.json

key - name
+--------------+------------------------------------------------------------------------+-------------+-----------+-------------+
| Field        | Description                                                            | Mandatory   | Default   | Reference   |
+==============+========================================================================+=============+===========+=============+
| dhcp_servers | The field contains list of unique membersConfigure the dhcp v4 servers |             |           |             |
+--------------+------------------------------------------------------------------------+-------------+-----------+-------------+

"""

dscp_to_tc_table_field_output="""\

DSCP_TO_TC_MAP
Description: DSCP_TO_TC_MAP part of config_db.json

key - name
+---------+------------------------------------------------------+-------------+-----------+-------------+
| Field   | Description                                          | Mandatory   | Default   | Reference   |
+=========+======================================================+=============+===========+=============+
| name    |                                                      |             |           |             |
+---------+------------------------------------------------------+-------------+-----------+-------------+
| dscp    | This field is for storing mapping between two fields |             |           |             |
+---------+------------------------------------------------------+-------------+-----------+-------------+
| tc      | This field is for storing mapping between two fields |             |           |             |
+---------+------------------------------------------------------+-------------+-----------+-------------+

"""

acl_rule_table_field_output="""\

ACL_RULE
Description: ACL_RULE part of config_db.json

key - ACL_TABLE_NAME:RULE_NAME
+-----------+-------------------------------------------------+-------------+-----------+-------------+
| Field     | Description                                     | Mandatory   | Default   | Reference   |
+===========+=================================================+=============+===========+=============+
| ICMP_TYPE | Mutually exclusive in group icmp                |             |           |             |
|           | when IP_TYPE in ANY,IP,IPV4,IPv4ANY,IPV4ANY,ARP |             |           |             |
+-----------+-------------------------------------------------+-------------+-----------+-------------+

"""

snmp_table_output="""\

SNMP

key - CONTACT
+---------+----------------------+-------------+-----------+-------------+
| Field   | Description          | Mandatory   | Default   | Reference   |
+=========+======================+=============+===========+=============+
| Contact | SNMP System Contact. |             |           |             |
+---------+----------------------+-------------+-----------+-------------+
key - LOCATION
+----------+-----------------------+-------------+-----------+-------------+
| Field    | Description           | Mandatory   | Default   | Reference   |
+==========+=======================+=============+===========+=============+
| Location | SNMP System Location. |             |           |             |
+----------+-----------------------+-------------+-----------+-------------+

"""

class TestCfgHelp(TestCase):

    def setUp(self):
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        self.script_file = os.path.join(self.test_dir, '..', 'sonic-cfg-help')

    def run_script(self, argument):
        print('\n    Running sonic-cfg-help ' + ' '.join(argument))
        output = subprocess.check_output([self.script_file] + argument)

        output = output.decode()

        linecount = output.strip().count('\n')
        if linecount <= 0:
            print('    Output: ' + output.strip())
        else:
            print('    Output: ({0} lines, {1} bytes)'.format(linecount + 1, len(output)))
        return output

    def test_dummy_run(self):
        argument = []
        output = self.run_script(argument)

        options = []
        options.append(("-t", "--table", "Table name"))
        options.append(("-f", "--field", "Field"))
        options.append(("-p", "--print_format", "Print format"))
        options.append(("-a", "--all", "Print all tables"))

        for option in options:
            with self.subTest(option=option):
                self.assertRegex(output, f"(?s:.)*[^\\w]{option[0]}(?s:.)*")
                self.assertRegex(output, f"(?s:.)*[^\\w]{option[1]}(?s:.)*")
                self.assertRegex(output, f"(?s:.)*\\b{option[2]}(?s:.)*")

    def test_single_table(self):
        argument = ['-t', 'AUTO_TECHSUPPORT']
        output = self.run_script(argument)
        self.assertEqual(output, techsupport_table_output)

    def test_single_field(self):
        argument = ['-t', 'AUTO_TECHSUPPORT', '-f', 'state']
        output = self.run_script(argument)
        self.assertEqual(output, techsupport_table_field_output)

    def test_leaf_list(self):
        argument = ['-t', 'VLAN', '-f', 'dhcp_servers']
        output = self.run_script(argument)
        self.assertEqual(output, vlan_table_field_output)

    def test_leaf_list_map(self):
        argument = ['-t', 'DSCP_TO_TC_MAP']
        output = self.run_script(argument)
        self.maxDiff = None
        self.assertEqual(output, dscp_to_tc_table_field_output)

    def test_when_condition(self):
        argument = ['-t', 'ACL_RULE', '-f', 'ICMP_TYPE']
        output = self.run_script(argument)
        self.assertEqual(output, acl_rule_table_field_output)

    def test_nested_container(self):
        argument = ['-t', 'SNMP']
        output = self.run_script(argument)
        self.assertEqual(output, snmp_table_output)
