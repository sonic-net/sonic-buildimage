"""
    Unit test cases for system health checker. The current test case contains:
        1. test_user_defined_checker mocks the output of a user defined checker and verify class UserDefinedChecker
        2. test_service_checker mocks the output of monit service and verify class ServiceChecker
        3. test_hardware_checker mocks the hardware status data in db and verify class HardwareChecker
        4. Mocks and tests the system ready status and verify class Sysmonitor
    And there are class that are not covered by unit test. These class will be covered by sonic-mgmt regression test.
        1. HealthDaemon
        2. HealthCheckerManager
        3. Config
"""
import copy
import os
import sys
from imp import load_source
from swsscommon import swsscommon

from mock import Mock, MagicMock, patch
from sonic_py_common import device_info

from .mock_connector import MockConnector

swsscommon.SonicV2Connector = MockConnector

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
scripts_path = os.path.join(modules_path, 'scripts')
sys.path.insert(0, modules_path)
sys.path.insert(0, scripts_path)
from health_checker import utils
from health_checker.config import Config
from health_checker.hardware_checker import HardwareChecker
from health_checker.health_checker import HealthChecker
from health_checker.manager import HealthCheckerManager
from health_checker.service_checker import ServiceChecker
from health_checker.user_defined_checker import UserDefinedChecker
from health_checker.sysmonitor import Sysmonitor
from health_checker.sysmonitor import MonitorStateDbTask
from health_checker.sysmonitor import MonitorSystemBusTask

load_source('healthd', os.path.join(scripts_path, 'healthd'))
from healthd import HealthDaemon

mock_supervisorctl_output = """
snmpd                       RUNNING   pid 67, uptime 1:03:56
snmp-subagent               EXITED    Oct 19 01:53 AM
"""
device_info.get_platform = MagicMock(return_value='unittest')


def setup():
    if os.path.exists(ServiceChecker.CRITICAL_PROCESS_CACHE):
        os.remove(ServiceChecker.CRITICAL_PROCESS_CACHE)


@patch('health_checker.utils.run_command')
def test_user_defined_checker(mock_run):
    mock_run.return_value = ''

    checker = UserDefinedChecker('')
    checker.check(None)
    assert checker._info[str(checker)][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK

    checker.reset()
    assert len(checker._info) == 0

    mock_run.return_value = '\n\n\n'
    checker.check(None)
    assert checker._info[str(checker)][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK

    valid_output = 'MyCategory\nDevice1:OK\nDevice2:Device2 is broken\n'
    mock_run.return_value = valid_output
    checker.check(None)
    assert checker.get_category() == 'MyCategory'
    assert 'Device1' in checker._info
    assert 'Device2' in checker._info
    assert checker._info['Device1'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_OK
    assert checker._info['Device2'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK


@patch('swsscommon.swsscommon.ConfigDBConnector.connect', MagicMock())
@patch('health_checker.service_checker.ServiceChecker._get_container_folder', MagicMock(return_value=test_path))
@patch('sonic_py_common.multi_asic.is_multi_asic', MagicMock(return_value=False))
@patch('docker.DockerClient')
@patch('health_checker.utils.run_command')
@patch('swsscommon.swsscommon.ConfigDBConnector')
def test_service_checker_single_asic(mock_config_db, mock_run, mock_docker_client):
    mock_db_data = MagicMock()
    mock_get_table = MagicMock()
    mock_db_data.get_table = mock_get_table
    mock_config_db.return_value = mock_db_data
    mock_get_table.return_value = {
        'snmp': {
            'state': 'enabled',
            'has_global_scope': 'True',
            'has_per_asic_scope': 'False',

        }
    }
    mock_containers = MagicMock()
    mock_snmp_container = MagicMock()
    mock_snmp_container.name = 'snmp'
    mock_containers.list = MagicMock(return_value=[mock_snmp_container])
    mock_docker_client_object = MagicMock()
    mock_docker_client.return_value = mock_docker_client_object
    mock_docker_client_object.containers = mock_containers

    mock_run.return_value = mock_supervisorctl_output

    checker = ServiceChecker()
    assert checker.get_category() == 'Services'
    config = Config()
    checker.check(config)
    assert 'snmp:snmpd' in checker._info
    assert checker._info['snmp:snmpd'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_OK

    assert 'snmp:snmp-subagent' in checker._info
    assert checker._info['snmp:snmp-subagent'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK

    mock_get_table.return_value = {
        'new_service': {
            'state': 'enabled',
            'has_global_scope': 'True',
            'has_per_asic_scope': 'False',
        },
        'snmp': {
            'state': 'enabled',
            'has_global_scope': 'True',
            'has_per_asic_scope': 'False',

        }
    }
    mock_ns_container = MagicMock()
    mock_ns_container.name = 'new_service'
    mock_containers.list = MagicMock(return_value=[mock_snmp_container, mock_ns_container])
    checker.check(config)
    assert 'new_service' in checker.container_critical_processes

    assert 'new_service:snmpd' in checker._info
    assert checker._info['new_service:snmpd'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_OK

    assert 'new_service:snmp-subagent' in checker._info
    assert checker._info['new_service:snmp-subagent'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK

    mock_containers.list = MagicMock(return_value=[mock_snmp_container])
    checker.check(config)
    assert 'new_service' in checker._info
    assert checker._info['new_service'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK

    mock_containers.list = MagicMock(return_value=[mock_snmp_container, mock_ns_container])
    mock_run.return_value = None
    checker.check(config)
    assert 'new_service:snmpd' in checker._info
    assert checker._info['new_service:snmpd'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK

    assert 'new_service:snmp-subagent' in checker._info
    assert checker._info['new_service:snmp-subagent'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK

    origin_container_critical_processes = copy.deepcopy(checker.container_critical_processes)
    checker.save_critical_process_cache()
    checker.load_critical_process_cache()
    assert origin_container_critical_processes == checker.container_critical_processes



@patch('swsscommon.swsscommon.ConfigDBConnector.connect', MagicMock())
@patch('health_checker.service_checker.ServiceChecker._get_container_folder', MagicMock(return_value=test_path))
@patch('health_checker.utils.run_command', MagicMock(return_value=mock_supervisorctl_output))
@patch('sonic_py_common.multi_asic.get_num_asics', MagicMock(return_value=3))
@patch('sonic_py_common.multi_asic.is_multi_asic', MagicMock(return_value=True))
@patch('sonic_py_common.multi_asic.get_namespace_list', MagicMock(return_value=[str(x) for x in range(3)]))
@patch('sonic_py_common.multi_asic.get_current_namespace', MagicMock(return_value=''))
@patch('docker.DockerClient')
@patch('swsscommon.swsscommon.ConfigDBConnector')
def test_service_checker_multi_asic(mock_config_db, mock_docker_client):
    mock_db_data = MagicMock()
    mock_db_data.get_table = MagicMock()
    mock_config_db.return_value = mock_db_data

    mock_db_data.get_table.return_value = {
        'snmp': {
            'state': 'enabled',
            'has_global_scope': 'True',
            'has_per_asic_scope': 'True',

        }
    }

    mock_containers = MagicMock()
    mock_snmp_container = MagicMock()
    mock_snmp_container.name = 'snmp'
    list_return_value = [mock_snmp_container]
    for i in range(3):
        mock_container = MagicMock()
        mock_container.name = 'snmp' + str(i)
        list_return_value.append(mock_container)

    mock_containers.list = MagicMock(return_value=list_return_value)
    mock_docker_client_object = MagicMock()
    mock_docker_client.return_value = mock_docker_client_object
    mock_docker_client_object.containers = mock_containers

    checker = ServiceChecker()

    config = Config()
    checker.check(config)
    assert 'snmp' in checker.container_critical_processes
    assert 'snmp:snmpd' in checker._info
    assert checker._info['snmp:snmpd'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_OK
    assert 'snmp0:snmpd' in checker._info
    assert checker._info['snmp0:snmpd'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_OK
    assert 'snmp1:snmpd' in checker._info
    assert checker._info['snmp1:snmpd'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_OK
    assert 'snmp2:snmpd' in checker._info
    assert checker._info['snmp2:snmpd'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_OK

    assert 'snmp:snmp-subagent' in checker._info
    assert checker._info['snmp:snmp-subagent'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK
    assert 'snmp0:snmp-subagent' in checker._info
    assert checker._info['snmp0:snmp-subagent'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK
    assert 'snmp1:snmp-subagent' in checker._info
    assert checker._info['snmp1:snmp-subagent'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK
    assert 'snmp2:snmp-subagent' in checker._info
    assert checker._info['snmp2:snmp-subagent'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK


@patch('swsscommon.swsscommon.ConfigDBConnector', MagicMock())
@patch('swsscommon.swsscommon.ConfigDBConnector.connect', MagicMock())
@patch('health_checker.service_checker.ServiceChecker.check_by_monit', MagicMock())
@patch('docker.DockerClient')
@patch('swsscommon.swsscommon.ConfigDBConnector.get_table')
def test_service_checker_no_critical_process(mock_get_table, mock_docker_client):
    mock_get_table.return_value = {
        'snmp': {
            'state': 'enabled',
            'has_global_scope': 'True',
            'has_per_asic_scope': 'True',

        }
    }
    mock_containers = MagicMock()
    mock_containers.list = MagicMock(return_value=[])
    mock_docker_client_object = MagicMock()
    mock_docker_client.return_value = mock_docker_client_object
    mock_docker_client_object.containers = mock_containers

    checker = ServiceChecker()
    config = Config()
    checker.check(config)
    assert 'system' in checker._info
    assert checker._info['system'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK

@patch('health_checker.service_checker.ServiceChecker.check_services', MagicMock())
@patch('health_checker.utils.run_command')
def test_service_checker_check_by_monit(mock_run):
    return_value = 'Monit 5.20.0 uptime: 3h 54m\n' \
                   'Service Name                     Status                      Type\n' \
                   'sonic                            Running                     System\n' \
                   'sonic1                           Not running                 System\n' \
                   'telemetry                        Does not exist              Process\n' \
                   'orchagent                        Running                     Process\n' \
                   'root-overlay                     Accessible                  Filesystem\n' \
                   'var-log                          Is not accessible           Filesystem\n'
    mock_run.side_effect = ['active', return_value]
    checker = ServiceChecker()
    config = Config()
    checker.check(config)
    assert 'sonic' in checker._info
    assert checker._info['sonic'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_OK

    assert 'sonic1' in checker._info
    assert checker._info['sonic1'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK

    assert 'orchagent' in checker._info
    assert checker._info['orchagent'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_OK

    assert 'telemetry' in checker._info
    assert checker._info['telemetry'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK

    assert 'root-overlay' in checker._info
    assert checker._info['root-overlay'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_OK

    assert 'var-log' in checker._info
    assert checker._info['var-log'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK


def test_hardware_checker():
    MockConnector.data.update({
        'TEMPERATURE_INFO|ASIC': {
            'temperature': '20',
            'high_threshold': '21'
        }
    })

    MockConnector.data.update({
        'FAN_INFO|fan1': {
            'presence': 'True',
            'status': 'True',
            'speed': '60',
            'speed_target': '60',
            'speed_tolerance': '20',
            'direction': 'intake'
        },
        'FAN_INFO|fan2': {
            'presence': 'False',
            'status': 'True',
            'speed': '60',
            'speed_target': '60',
            'speed_tolerance': '20'
        },
        'FAN_INFO|fan3': {
            'presence': 'True',
            'status': 'False',
            'speed': '60',
            'speed_target': '60',
            'speed_tolerance': '20'
        },
        'FAN_INFO|fan4': {
            'presence': 'True',
            'status': 'True',
            'speed': '20',
            'speed_target': '60',
            'speed_tolerance': '20'
        },
        'FAN_INFO|fan5': {
            'presence': 'True',
            'status': 'True',
            'speed': '60',
            'speed_target': '60',
            'speed_tolerance': '20',
            'direction': 'exhaust'
        }
    })

    MockConnector.data.update({
        'PSU_INFO|PSU 1': {
            'presence': 'True',
            'status': 'True',
            'temp': '55',
            'temp_threshold': '100',
            'voltage': '10',
            'voltage_min_threshold': '8',
            'voltage_max_threshold': '15',
        },
        'PSU_INFO|PSU 2': {
            'presence': 'False',
            'status': 'True',
            'temp': '55',
            'temp_threshold': '100',
            'voltage': '10',
            'voltage_min_threshold': '8',
            'voltage_max_threshold': '15',
        },
        'PSU_INFO|PSU 3': {
            'presence': 'True',
            'status': 'False',
            'temp': '55',
            'temp_threshold': '100',
            'voltage': '10',
            'voltage_min_threshold': '8',
            'voltage_max_threshold': '15',
        },
        'PSU_INFO|PSU 4': {
            'presence': 'True',
            'status': 'True',
            'temp': '101',
            'temp_threshold': '100',
            'voltage': '10',
            'voltage_min_threshold': '8',
            'voltage_max_threshold': '15',
        },
        'PSU_INFO|PSU 5': {
            'presence': 'True',
            'status': 'True',
            'temp': '55',
            'temp_threshold': '100',
            'voltage': '10',
            'voltage_min_threshold': '12',
            'voltage_max_threshold': '15',
        },
        'PSU_INFO|PSU 6': {
            'presence': 'True',
            'status': 'True',
            'temp': '55',
            'temp_threshold': '100',
            'voltage': '12',
            'voltage_min_threshold': '12',
            'voltage_max_threshold': '15',
            'power_overload': 'True',
            'power': '101.0',
            'power_critical_threshold': '100.0',
            'power_warning_suppress_threshold': '90.0'
        },
        'PSU_INFO|PSU 7': {
            'presence': 'True',
            'status': 'True',
            'temp': '55',
            'temp_threshold': '100',
            'voltage': '12',
            'voltage_min_threshold': '12',
            'voltage_max_threshold': '15',
            'power_overload': 'True',
            'power': '101.0'
        }
    })

    checker = HardwareChecker()
    assert checker.get_category() == 'Hardware'
    config = Config()
    checker.check(config)

    assert 'ASIC' in checker._info
    assert checker._info['ASIC'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_OK

    assert 'fan1' in checker._info
    assert checker._info['fan1'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_OK

    assert 'fan2' in checker._info
    assert checker._info['fan2'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK

    assert 'fan3' in checker._info
    assert checker._info['fan3'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK

    assert 'fan4' in checker._info
    assert checker._info['fan4'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK

    assert 'fan5' in checker._info
    assert checker._info['fan5'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK
    assert checker._info['fan5'][HealthChecker.INFO_FIELD_OBJECT_MSG] == 'fan5 direction exhaust is not aligned with fan1 direction intake'

    assert 'PSU 1' in checker._info
    assert checker._info['PSU 1'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_OK

    assert 'PSU 2' in checker._info
    assert checker._info['PSU 2'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK

    assert 'PSU 3' in checker._info
    assert checker._info['PSU 3'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK

    assert 'PSU 4' in checker._info
    assert checker._info['PSU 4'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK

    assert 'PSU 5' in checker._info
    assert checker._info['PSU 5'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK

    assert 'PSU 6' in checker._info
    assert checker._info['PSU 6'][HealthChecker.INFO_FIELD_OBJECT_MSG] == 'System power exceeds threshold (100.0w)'
    assert checker._info['PSU 6'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK

    assert 'PSU 7' in checker._info
    assert checker._info['PSU 7'][HealthChecker.INFO_FIELD_OBJECT_STATUS] == HealthChecker.STATUS_NOT_OK
    assert checker._info['PSU 7'][HealthChecker.INFO_FIELD_OBJECT_MSG] == 'System power exceeds threshold but power_critical_threshold is invalid'


def test_config():
    config = Config()
    config._config_file = os.path.join(test_path, Config.CONFIG_FILE)

    assert config.config_file_exists()
    config.load_config()
    assert config.interval == 60
    assert 'dummy_service' in config.ignore_services
    assert 'psu.voltage' in config.ignore_devices
    assert len(config.user_defined_checkers) == 0

    assert config.get_led_color('fault') == 'orange'
    assert config.get_led_color('normal') == 'green'
    assert config.get_led_color('booting') == 'orange_blink'
    assert config.get_bootup_timeout() == 300

    config._reset()
    assert not config.ignore_services
    assert not config.ignore_devices
    assert not config.user_defined_checkers
    assert not config.config_data

    assert config.get_led_color('fault') == 'red'
    assert config.get_led_color('normal') == 'green'
    assert config.get_led_color('booting') == 'red'

    config._last_mtime  = 1
    config._config_file = 'notExistFile'
    config.load_config()
    assert not config._last_mtime


@patch('swsscommon.swsscommon.ConfigDBConnector', MagicMock())
@patch('swsscommon.swsscommon.ConfigDBConnector.connect', MagicMock())
@patch('health_checker.service_checker.ServiceChecker.check', MagicMock())
@patch('health_checker.hardware_checker.HardwareChecker.check', MagicMock())
@patch('health_checker.user_defined_checker.UserDefinedChecker.check', MagicMock())
@patch('swsscommon.swsscommon.ConfigDBConnector.get_table', MagicMock())
@patch('health_checker.user_defined_checker.UserDefinedChecker.get_category', MagicMock(return_value='UserDefine'))
@patch('health_checker.user_defined_checker.UserDefinedChecker.get_info')
@patch('health_checker.service_checker.ServiceChecker.get_info')
@patch('health_checker.hardware_checker.HardwareChecker.get_info')
def test_manager(mock_hw_info, mock_service_info, mock_udc_info):
    chassis = MagicMock()
    chassis.set_status_led = MagicMock()

    manager = HealthCheckerManager()
    manager.config.user_defined_checkers = ['some check']
    assert len(manager._checkers) == 2

    mock_hw_info.return_value = {
        'ASIC': {
            'type': 'ASIC',
            'message': '',
            'status': 'OK'
        },
        'fan1': {
            'type': 'Fan',
            'message': '',
            'status': 'OK'
        },
    }
    mock_service_info.return_value = {
        'snmp:snmpd': {
            'type': 'Process',
            'message': '',
            'status': 'OK'
        }
    }
    mock_udc_info.return_value = {
        'udc': {
            'type': 'Database',
            'message': '',
            'status': 'OK'
        }
    }
    stat = manager.check(chassis)
    assert 'Services' in stat
    assert stat['Services']['snmp:snmpd']['status'] == 'OK'

    assert 'Hardware' in stat
    assert stat['Hardware']['ASIC']['status'] == 'OK'
    assert stat['Hardware']['fan1']['status'] == 'OK'

    assert 'UserDefine' in stat
    assert stat['UserDefine']['udc']['status'] == 'OK'

    mock_hw_info.side_effect = RuntimeError()
    mock_service_info.side_effect = RuntimeError()
    mock_udc_info.side_effect = RuntimeError()
    stat = manager.check(chassis)
    assert 'Internal' in stat
    assert stat['Internal']['ServiceChecker']['status'] == 'Not OK'
    assert stat['Internal']['HardwareChecker']['status'] == 'Not OK'
    assert stat['Internal']['UserDefinedChecker - some check']['status'] == 'Not OK'

    chassis.set_status_led.side_effect = NotImplementedError()
    manager._set_system_led(chassis)

    chassis.set_status_led.side_effect = RuntimeError()
    manager._set_system_led(chassis)

def test_utils():
    output = utils.run_command('some invalid command')
    assert not output

    output = utils.run_command('ls')
    assert output


@patch('swsscommon.swsscommon.ConfigDBConnector.connect', MagicMock())
@patch('sonic_py_common.multi_asic.is_multi_asic', MagicMock(return_value=False))
@patch('docker.DockerClient')
@patch('health_checker.utils.run_command')
@patch('swsscommon.swsscommon.ConfigDBConnector')
def test_get_all_service_list(mock_config_db, mock_run, mock_docker_client):
    mock_db_data = MagicMock()
    mock_get_table = MagicMock()
    mock_db_data.get_table = mock_get_table
    mock_config_db.return_value = mock_db_data
    mock_get_table.return_value = {
        'radv': {
            'state': 'enabled',
            'has_global_scope': 'True',
            'has_per_asic_scope': 'False',
        },
        'bgp': {
            'state': 'enabled',
            'has_global_scope': 'True',
            'has_per_asic_scope': 'False',
        },
        'pmon': {
            'state': 'disabled',
            'has_global_scope': 'True',
            'has_per_asic_scope': 'False',
        }
    }
    sysmon = Sysmonitor()
    print("mock get table:{}".format(mock_get_table.return_value))
    result = sysmon.get_all_service_list()
    print("result get all service list:{}".format(result))
    assert 'radv.service' in result
    assert 'pmon.service' not in result


@patch('swsscommon.swsscommon.ConfigDBConnector.connect', MagicMock())
@patch('sonic_py_common.multi_asic.is_multi_asic', MagicMock(return_value=False))
@patch('docker.DockerClient')
@patch('health_checker.utils.run_command')
@patch('swsscommon.swsscommon.ConfigDBConnector')
def test_get_app_ready_status(mock_config_db, mock_run, mock_docker_client):
    mock_db_data = MagicMock()
    mock_get_table = MagicMock()
    mock_db_data.get_table = mock_get_table
    mock_config_db.return_value = mock_db_data
    mock_get_table.return_value = {
        'radv': {
            'state': 'enabled',
            'has_global_scope': 'True',
            'has_per_asic_scope': 'False',
            'check_up_status': 'True'
        },
        'bgp': {
            'state': 'enabled',
            'has_global_scope': 'True',
            'has_per_asic_scope': 'False',
            'check_up_status': 'True'
        },
        'snmp': {
            'state': 'enabled',
            'has_global_scope': 'True',
            'has_per_asic_scope': 'False',
            'check_up_status': 'False'
        }
    }

    MockConnector.data.update({
        'FEATURE|radv': {
            'up_status': 'True',
            'fail_reason': '-',
            'update_time': '-'
        },
        'FEATURE|bgp': {
            'up_status': 'False',
            'fail_reason': 'some error',
            'update_time': '-'
        }})

    sysmon = Sysmonitor()
    result = sysmon.get_app_ready_status('radv')
    print(result)
    assert 'Up' in result
    result = sysmon.get_app_ready_status('bgp')
    print(result)
    assert 'Down' in result
    result = sysmon.get_app_ready_status('snmp')
    print(result)
    assert 'Up' in result


mock_srv_props={
'mock_radv.service':{'Type': 'simple', 'Result': 'success', 'Id': 'mock_radv.service', 'LoadState': 'loaded', 'ActiveState': 'active', 'SubState': 'running', 'UnitFileState': 'enabled'},
'mock_bgp.service':{'Type': 'simple', 'Result': 'success', 'Id': 'mock_bgp.service', 'LoadState': 'loaded', 'ActiveState': 'inactive', 'SubState': 'dead', 'UnitFileState': 'enabled'}
}

@patch('health_checker.sysmonitor.Sysmonitor.get_all_service_list', MagicMock(return_value=['mock_snmp.service', 'mock_bgp.service', 'mock_ns.service']))
@patch('health_checker.sysmonitor.Sysmonitor.run_systemctl_show', MagicMock(return_value=mock_srv_props['mock_bgp.service']))
@patch('health_checker.sysmonitor.Sysmonitor.get_app_ready_status', MagicMock(return_value=('Down','-','-')))
@patch('health_checker.sysmonitor.Sysmonitor.post_unit_status', MagicMock())
@patch('health_checker.sysmonitor.Sysmonitor.publish_system_status', MagicMock())
def test_check_unit_status():
    sysmon = Sysmonitor()
    sysmon.check_unit_status('mock_bgp.service')
    assert 'mock_bgp.service' in sysmon.dnsrvs_name


@patch('health_checker.sysmonitor.Sysmonitor.get_all_service_list', MagicMock(side_effect=[
    ['mock_snmp.service', 'mock_bgp.service', 'mock_ns.service'],
    ['mock_snmp.service', 'mock_ns.service']
]))
@patch('health_checker.sysmonitor.Sysmonitor.run_systemctl_show', MagicMock(return_value=mock_srv_props['mock_bgp.service']))
@patch('health_checker.sysmonitor.Sysmonitor.get_app_ready_status', MagicMock(return_value=('Down','-','-')))
@patch('health_checker.sysmonitor.Sysmonitor.post_unit_status', MagicMock())
@patch('health_checker.sysmonitor.Sysmonitor.print_console_message', MagicMock())
def test_system_status_up_after_service_removed():
    sysmon = Sysmonitor()
    sysmon.publish_system_status('UP')

    sysmon.check_unit_status('mock_bgp.service')
    assert 'mock_bgp.service' in sysmon.dnsrvs_name
    result = swsscommon.SonicV2Connector.get(MockConnector, 0, "SYSTEM_READY|SYSTEM_STATE", 'Status')
    print("system status result before service was removed from system: {}".format(result))
    assert result == "DOWN"

    sysmon.check_unit_status('mock_bgp.service')
    assert 'mock_bgp.service' not in sysmon.dnsrvs_name
    result = swsscommon.SonicV2Connector.get(MockConnector, 0, "SYSTEM_READY|SYSTEM_STATE", 'Status')
    print("system status result after service was removed from system: {}".format(result))
    assert result == "UP"


@patch('health_checker.sysmonitor.Sysmonitor.get_all_service_list', MagicMock(return_value=['mock_snmp.service']))
def test_check_unit_status_timer():
    sysmon = Sysmonitor()
    sysmon.state_db = MagicMock()
    sysmon.state_db.exists = MagicMock(return_value=1)
    sysmon.state_db.delete = MagicMock()
    sysmon.check_unit_status('mock_snmp.timer')
    assert not sysmon.state_db.delete.called


@patch('health_checker.sysmonitor.Sysmonitor.run_systemctl_show', MagicMock(return_value=mock_srv_props['mock_radv.service']))
@patch('health_checker.sysmonitor.Sysmonitor.get_app_ready_status', MagicMock(return_value=('Up','-','-')))
@patch('health_checker.sysmonitor.Sysmonitor.post_unit_status', MagicMock())
def test_get_unit_status_ok():
    sysmon = Sysmonitor()
    result = sysmon.get_unit_status('mock_radv.service')
    print("get_unit_status:{}".format(result))
    assert result == 'OK'


@patch('health_checker.sysmonitor.Sysmonitor.run_systemctl_show', MagicMock(return_value=mock_srv_props['mock_bgp.service']))
@patch('health_checker.sysmonitor.Sysmonitor.get_app_ready_status', MagicMock(return_value=('Up','-','-')))
@patch('health_checker.sysmonitor.Sysmonitor.post_unit_status', MagicMock())
def test_get_unit_status_not_ok():
    sysmon = Sysmonitor()
    result = sysmon.get_unit_status('mock_bgp.service')
    print("get_unit_status:{}".format(result))
    assert result == 'NOT OK'


@patch('health_checker.sysmonitor.Sysmonitor.get_all_service_list', MagicMock(return_value=['mock_snmp.service', 'mock_ns.service']))
@patch('health_checker.sysmonitor.Sysmonitor.get_unit_status', MagicMock(return_value= 'OK'))
@patch('health_checker.sysmonitor.Sysmonitor.publish_system_status', MagicMock())
@patch('health_checker.sysmonitor.Sysmonitor.post_unit_status', MagicMock())
@patch('health_checker.sysmonitor.Sysmonitor.get_app_ready_status', MagicMock(return_value='Up'))
def test_get_all_system_status_ok():
    sysmon = Sysmonitor()
    result = sysmon.get_all_system_status()
    print("result:{}".format(result))
    assert result == 'UP'


@patch('health_checker.sysmonitor.Sysmonitor.get_all_service_list', MagicMock(return_value=['mock_snmp.service', 'mock_ns.service']))
@patch('health_checker.sysmonitor.Sysmonitor.get_unit_status', MagicMock(return_value= 'NOT OK'))
@patch('health_checker.sysmonitor.Sysmonitor.publish_system_status', MagicMock())
@patch('health_checker.sysmonitor.Sysmonitor.post_unit_status', MagicMock())
@patch('health_checker.sysmonitor.Sysmonitor.get_app_ready_status', MagicMock(return_value='Up'))
def test_get_all_system_status_not_ok():
    sysmon = Sysmonitor()
    result = sysmon.get_all_system_status()
    print("result:{}".format(result))
    assert result == 'DOWN'

def test_post_unit_status():
    sysmon = Sysmonitor()
    sysmon.post_unit_status("mock_bgp", 'OK', 'Down', 'mock reason', '-')
    result = swsscommon.SonicV2Connector.get_all(MockConnector, 0, 'ALL_SERVICE_STATUS|mock_bgp')
    print(result)
    assert result['service_status'] == 'OK'
    assert result['app_ready_status'] == 'Down'
    assert result['fail_reason'] == 'mock reason'

def test_post_system_status():
    sysmon = Sysmonitor()
    sysmon.post_system_status("UP")
    result = swsscommon.SonicV2Connector.get(MockConnector, 0, "SYSTEM_READY|SYSTEM_STATE", 'Status')
    print("post system status result:{}".format(result))
    assert result == "UP"

    sysmon.post_system_status("DOWN")
    result = swsscommon.SonicV2Connector.get(MockConnector, 0, "SYSTEM_READY|SYSTEM_STATE", 'Status')
    print("post system status result:{}".format(result))
    assert result == "DOWN"

@patch('health_checker.sysmonitor.Sysmonitor.print_console_message', MagicMock())
@patch('health_checker.sysmonitor.Sysmonitor.post_system_status', MagicMock())
def test_publish_system_status_allowed_status():
    sysmon = Sysmonitor()
    sysmon.publish_system_status('UP')
    sysmon.publish_system_status('DOWN')
    
    expected_calls = [
        (("UP",), {}),
        (("DOWN",), {})
    ]
    for call_args in sysmon.post_system_status.call_args_list:
        assert call_args in expected_calls

@patch('health_checker.sysmonitor.Sysmonitor.print_console_message', MagicMock())
def test_publish_system_status():
    sysmon = Sysmonitor()
    sysmon.publish_system_status('UP')
    result = swsscommon.SonicV2Connector.get(MockConnector, 0, "SYSTEM_READY|SYSTEM_STATE", 'Status')
    assert result == "UP" 

@patch('health_checker.sysmonitor.Sysmonitor.get_all_system_status', test_get_all_system_status_ok())
@patch('health_checker.sysmonitor.Sysmonitor.publish_system_status', test_publish_system_status())
def test_update_system_status():
    sysmon = Sysmonitor()
    sysmon.update_system_status()
    result = swsscommon.SonicV2Connector.get(MockConnector, 0, "SYSTEM_READY|SYSTEM_STATE", 'Status')
    assert result == "UP"

from sonic_py_common.task_base import ProcessTaskBase
import multiprocessing
mpmgr = multiprocessing.Manager()

myQ = mpmgr.Queue()
def test_monitor_statedb_task():
    sysmon = MonitorStateDbTask(myQ)
    sysmon.SubscriberStateTable = MagicMock()
    sysmon.task_run()
    assert sysmon._task_process is not None
    sysmon.task_stop()

@patch('health_checker.sysmonitor.MonitorSystemBusTask.subscribe_sysbus', MagicMock())
def test_monitor_sysbus_task():
    sysmon = MonitorSystemBusTask(myQ)
    sysmon.SubscriberStateTable = MagicMock()
    sysmon.task_run()
    assert sysmon._task_process is not None
    sysmon.task_stop()

@patch('health_checker.sysmonitor.MonitorSystemBusTask.subscribe_sysbus', MagicMock())
@patch('health_checker.sysmonitor.MonitorStateDbTask.subscribe_statedb', MagicMock())
def test_system_service():
    sysmon = Sysmonitor()
    sysmon.task_run()
    assert sysmon._task_process is not None
    sysmon.task_stop()


def test_get_service_from_feature_table():
    sysmon = Sysmonitor()
    sysmon.config_db = MagicMock()
    sysmon.config_db.get_table = MagicMock()
    sysmon.config_db.get_table.side_effect = [
        {
            'bgp': {},
            'swss': {}
        },
        {
            'bgp': {'state': 'enabled'},
            'swss': {'state': 'disabled'}
        }
    ]
    dir_list = []
    sysmon.get_service_from_feature_table(dir_list)
    assert 'bgp.service' in dir_list
    assert 'swss.service' not in dir_list


@patch('healthd.time.time')
def test_healthd_check_interval(mock_time):
    daemon = HealthDaemon()
    manager = MagicMock()
    manager.check = MagicMock()
    manager.config = MagicMock()
    chassis = MagicMock()
    daemon._process_stat = MagicMock()
    daemon.stop_event = MagicMock()
    daemon.stop_event.wait = MagicMock()

    daemon.stop_event.wait.return_value = False
    manager.config.interval = 60
    mock_time.side_effect = [0, 3, 0, 61, 0, 1]
    assert daemon._run_checker(manager, chassis)
    daemon.stop_event.wait.assert_called_with(57)
    assert daemon._run_checker(manager, chassis)
    daemon.stop_event.wait.assert_called_with(1)

    daemon.stop_event.wait.return_value = True
    assert not daemon._run_checker(manager, chassis)
