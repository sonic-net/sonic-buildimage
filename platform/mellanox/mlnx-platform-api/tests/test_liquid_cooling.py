import os
import sys
from unittest import mock
from sonic_platform.liquid_cooling import LiquidCooling, LeakageSensor
from sonic_platform.utils import read_int_from_file

def test_leakage_sensor_init():
    sensor = LeakageSensor("leakage1", "/test/path")
    assert sensor.name == "leakage1"
    assert sensor.path == "/test/path"

def test_leakage_sensor_is_leak():
    sensor = LeakageSensor("leakage1", "/test/path")
    
    # Test when file exists and content is "1" (no leak)
    with mock.patch('os.path.exists') as mock_exists:
        with mock.patch('builtins.open', mock.mock_open(read_data="1")):
            mock_exists.return_value = True
            assert sensor.is_leak() is False

    # Test when file exists and content is "0" (leak detected)
    with mock.patch('os.path.exists') as mock_exists:
        with mock.patch('builtins.open', mock.mock_open(read_data="0")):
            mock_exists.return_value = True
            assert sensor.is_leak() is True

    # Test when file does not exist
    with mock.patch('os.path.exists') as mock_exists:
        mock_exists.return_value = False
        assert sensor.is_leak() == 'N/A'

def test_liquid_cooling_init():
    with mock.patch('os.path.exists') as mock_exists, \
         mock.patch('os.path.join', side_effect=lambda *args: "/".join(args)) as mock_join, \
         mock.patch('glob.glob') as mock_glob, \
         mock.patch('sonic_platform.utils.read_int_from_file', return_value=4) as mock_read_int:
        
        # Setup mock to simulate 4 leakage sensors
        mock_exists.side_effect = [True, True, True, True]
        mock_glob.return_value = [
            "/var/run/hw-management/system/leakage1",
            "/var/run/hw-management/system/leakage2", 
            "/var/run/hw-management/system/leakage3",
            "/var/run/hw-management/system/leakage4"
        ]

        liquid_cooling = LiquidCooling()
        
        # Verify the number of sensors initialized
        assert liquid_cooling.get_num_leak_sensors() == 4
        

        with mock.patch.object(liquid_cooling.leakage_sensors[3], 'is_leak', return_value=False) as mock_sensor1:
            sensors = liquid_cooling.get_leak_sensor_status()
            assert len(sensors) == 3
