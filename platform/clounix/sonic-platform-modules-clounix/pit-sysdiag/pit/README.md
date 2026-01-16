# `sysdiag` 设计说明
`sysdiag`是一套交换机软硬件测试工具集，目的是对交换机进行健康检查，生成测试报告，并支持环境类测试、研发测试和生产测试等。

`sysdiag`的设计理念是：

 * 测试项通用化，不依赖某硬件平台或特殊场景；
 * 测试项的具体测试流程需大家认可；
 * 测试项测试过程中需要输入的内容均采用读配置文件的形式；
 * 测试项中涉及平台数据获取的调用，均可通过`plugins`下的模块；

由于每个平台硬件设计的差异，可以通过平台层的配置文件做隔离。

### 1 整体架构

`sysdiag`整体架构如下图。`sysdiag`包括两部分`test_cases`和`tools`，其中`test_cases`是测试项集合，`tools`是平台工具。

```json
      ┌───────────────────────────────────────────────────────────┐
      │                        sysdiag                            │
      │  ┌────────────────┬───────────────┐      ┌─────────────┐  │
      │  │ test_caseA     │   fantool     │      │             │  │
      │  │ test_caseB     │   psutool     │      │  traffic    │  │
      │  │ test_caseZ     │   ...         │      │             │  │
      │  └────────────────┴───────────────┘      └─────────────┘  │
      │           ▲               ▲                ▲              │
      │           │               │                │              │
      └───────────┼───────────────┼────────────────┼──────────────┘
                  │               │                │
         ┌────────┴────────┬──────┴──────────┬─────┴───────────┐
middle   │                 │                 │                 │
abstract │   plugins       │ common_tools    │     bcmcmd      │
layer    │                 │                 │     ctc_shell   │
         └─────────────────┴─────────────────┴─────────────────┘


         ┌─────────────────────────────────┐ ┌─────────────────┐
         │                                 │ │                 │
kernel   │           hardware drivers      │ │     asic        │
space    │                                 │ │                 │
         └─────────────────────────────────┘ └─────────────────┘
```

### 2 `plugins`中间抽象层接口设计
`plugins`是`SONiC`下的平台抽象层，`sysdiag`可以利用并扩充此模块提供的接口作为平台数据来源。
下面列举的`plugins`模块属于硬件平台下的通用组件，如果`sysdiag`需要访问下面模块以外的信息，可以通过通用的第三方工具或者`nos`下的文件接口，比如：

* `CPU`：`/proc/cpuinfo`或者`dmidecode`
* `memory`：`/proc/meminfo`或者`dmidecode`
* `SSD`：`mmc-util`或者`smartctl`
* `RTC`：`/sys/class/rtc/rtc0/*`



#### 2.1 `sfputil.py`
`sfputil.py`用于光模块访问控制。测试过程中主要关注光模块`IO`状态、光模块`EEPROM`读写等。需要实现的接口如下：

``` python
class SfpUtil(SfpUtilBase):
    # 获取sfp端口列表
    @property
    def sfp_port_list(self):
        '''
        SFP Ports list
        @return LIST [1, 2, 3, ...]
        '''
        return LIST

    # 获取qsfp端口列表
    @property 
    def qsfp_ports(self):
        '''
        QSFP Ports list
        @return LIST [1, 2, 3, ...]
        '''
        return LIST

    # 获取光模块端口与eeprom映射字典
    @property
    def port_to_eeprom_mapping(self):
        '''
        dictionary where key=physical port index(integer), value=path to SFP EEPROM file(string)
        @return DICT {
            1: None,
            2: '/sys/bus/i2c/devices/10-0050/eeprom'
        }
        '''
        return DICT

    # 获取光模块presence
    def get_presence(self, port_index):
        return True/False

    # 获取光模块qsfp interrupt
    def get_interrupt(self, port_index):
        return True/False

    # 获取光模块qsfp low power mode
    def get_low_power_mode(self, port_index):
        return True/False
    def set_low_power_mode(self, port_index, enable):
        return True/False

    # 获取光模块qsfp reset
    def get_reset(self, port_index):
        return True/False
    def reset(self, port_index):
        return True/False

    # 获取光模块sfp tx fault
    def get_tx_fault(self, port_index):
        return True/False

    # 获取光模块sfp rx los
    def get_rx_los(self, port_index):
        return True/False

    # 获取光模块sfp tx disable
    def get_tx_disable(self, port_index):
        return True/False
    def set_tx_disable(self, port_index, enable):
        return True/False

    # Eload支持low speed signals monitor/control，需要实现set/get接口
    # For sfp
    def get_eload_tx_disable(self, port_index):
        return True/False
    def set_eload_tx_fault(self, port_index, enable):
        return True/False
    def set_eload_rx_los(self, port_index, enable):
        return True/False
    def set_eload_presence(self, port_index, enable): # sfp set mod_abs, qsfp set modprs
        return True/False
    # For qsfp
    def get_eload_low_power_mode(self, port_index):
        return True/False
    def get_eload_reset_changed(self, port_index):
        '''
            when set eload modprstL high, port cpld will reset qsfp;
            we need call below function to check wether reset changed or not
        '''
        return True/False
    def set_eload_interrupt(self, port_index, enable):
        return True/False
    # eload power setting
    def set_eload_power(self, port_index, value):
        return True/False
    # eload insertion counter
    def get_eload_insertion_counter(self, port_index):
        return integer

    # 获取光模块在位状态记录
    def get_plug_record(self):
        '''
        由于cpld中光模块在位状态记录是读清的，所以该接口需要一次性把数值拿出来。
        返回字典，key=physical port index(integer), value=True if plug happend else False,
        @return DICT {
            1: False, # 端口没有发生在位状态变化
            2: True,  # 端口发生过在位状态变化
        }
        ''' 
        return {
            1:True,
            2:False,
        }

    # 光模块事件上报
    def get_transceiver_change_event(self, timeout=0):
        return DICT
```

`sysdiag`测试项可以通过`sfputil.py`获取硬件平台下的端口形态配置，并对光模块进行诊断。


#### 2.2 `fanutil.py`
`fanutil.py`主要实现风扇访问控制，支持转速查看、占空比设置等。需要实现接口如下：

```python
class FanUtil(object):
    def __init__(self):
        pass

    # 获取设备支持的风扇数量
    def get_num_fans(self):
        """
        Retrieves the number of FANs supported on the device
        """
        return FAN_NUM
    
    # 获取风扇在位状态
    def get_fan_presence(self, index):
        """
        Retrieves the presence status of FAN defined by 1-based index <index>
        @index: An integer, 1-based index of the FAN
        @return: Boolean, True if FAN is plugged, False if not
        """
        return True/False
    
    # 获取风扇信息
    def get_all(self):
        """
        Retrieves the FANs information
        @return DICT
        """
        return {
            "Number": 1,
            "FAN1": {
                    "Present": True,
                    "Running": True,
                    "Speed": 12000.0,
                    "pwm": 124,
                    "LowThd": 5400.0,
                    "HighThd": 20000.0,
                    "SN": "serial_number_example",  # 'N/A'
                    "PN": "part_number_exampple",   # 'N/A'
                    "Status": True,
                    "AirFlow": "F2B",               # 'N/A'
                    }
            }

    # 修改风扇转速
    def set_speed(self, fan, pwm):
        # @fan: fan index;
        # @pwm: pwm value [0 ~ 100];
        return True/False

    # 设置风扇强制全速(最高优先级)
    def set_full_speed(self, enable):
        return True/False

    # 设置风扇控制模式(auto/manual)
    def set_mode(self, mode):
        return True/False

```

#### 2.3 `psuutil.py`
`psuuti.py`主要实现`PSU`状态获取。

```python
class PsuUtil(PsuBase):
    
    # 获取设备支持的PSU数量
    def get_num_psus(self):
        """
        Retrieves the number of PSUs supported on the device
        """
        return PSU_NUM

    # 获取PSU状态
    def get_psu_status(self, index):
        """
        Retrieves the operational status of PSU
        @index: An integer, 1-based index of PSU of which to query status
        @return: Boolean, True if PSU is operating properly, False if PSU is faulty
        """
        return True/False
    
    # 获取PSU信息
    def get_all(self):
        return {
            "Number": PSU_NUM,
            "PSU1": {
                    "Present": True,
                    "InputType": "AC" or "DC",
                    "InputStatus": True/False,
                    "OutputStatus": True/False,
                    "AirFlow": "F2B",               # 'N/A'
                    "SN": "serial_number_example",  # 'N/A'
                    "PN": "part_number_exampple",   # 'N/A'
                    "Vendor": "Vendor_name",        # 'N/A'
                    "HwVersion": "1.0",             # 硬件版本信息，'N/A'
                    "FwVersion": "1.0",             # 固件版本信息，'N/A'
                    "Fan": {
                        "Value": 1100,
                        "LowThd": 100,
                        "HighThd": 2000,
                        "Unit": "rpm"
                    },
                    "Temperature": {
                        "Value": 1100,
                        "LowThd": 100,
                        "HighThd": 2000,
                        "Unit": "C"
                    },
                    "InputsPower": {
                        "Value": 111.1,
                        "LowThd": 111.1,
                        "HighThd": 111.1,
                        "Unit": 'W',
                    },
                    "OutputsPower": {
                        "Value": 111.1,
                        "LowThd": 111.1,
                        "HighThd": 111.1,
                        "Unit": 'W',
                    },
                    "InputsVoltage": {
                        "Value": 111.1,
                        "LowThd": 111.1,
                        "HighThd": 111.1,
                        "Unit": 'V',
                    },
                    "OutputsVoltage": {
                        "Value": 111.1,
                        "LowThd": 111.1,
                        "HighThd": 111.1,
                        "Unit": 'V',
                    },
                    "InputsCurrent": {
                        "Value": 111.1,
                        "LowThd": 111.1,
                        "HighThd": 111.1,
                        "Unit": 'A',
                    },
                    "OutputsCurrent": {
                        "Value": 111.1,
                        "LowThd": 111.1,
                        "HighThd": 111.1,
                        "Unit": 'A',
                    }
            }
        }

    # 设置PSU风扇转速(PSU FAN的最低转速修改)
    def set_psu_fan_speed(self, psu_index, pwm):
        """
        set psu fan speed.
        @psu_index: An integer, 1-based index of PSU
        @pwm: An integer, rang is [0 ~ 100]
        """
        return True/False
    
```

#### 2.4 `sensorutil.py`
`sensorutil.py` 主要实现`sensor`的访问。接口如下：
```python
class SensorUtil(object):
    def __init__(self):
        pass

    # 获取设备支持的sensor数量
    def get_num_sensors(self):
        """
        Retrieves the number of Sensors supported on the device
        """
        return SENSOR_NUM

    # 获取所有sensor状态
    def get_all(self):
        return {
            "EMC1403-I2C-3-5C":{
                "Present":True,
                "CPU_TEMP": {
                    "Value": 12.0,
                    "LowThd": 54.0,
                    "HighThd": 65.0,
                    "Unit": "C",
                    "Description":"CPU TEMP"
                }，
                "Inlet_Temp": {
                    "Value": 12.0,
                    "LowThd": 54.0,
                    "HighThd": 65.0,
                    "Unit": "C",
                    "Description":"SMB Inlet TEMP"
                },
            },
            "ADC128D818-I2C-9-1F":{
                "Present":True,
                "P3V3_SW": {
                    "Value": 3.2,
                    "LowThd": 3.0,
                    "HighThd": 3.4,
                    "Unit": 'V',
                    "Description":""
                },
            },
            "PSU-I2C-4-59":{
                "Present":True,
                "PSU1_CIN": {
                    "Value": 23.0,
                    "LowThd": 0,
                    "HighThd": 25,
                    "Unit": 'A',
                    "Description":""
                },
                "PSU1_VIN": {
                    "Value": 220,
                    "LowThd": 182,
                    "HighThd": 290,
                    "Unit": 'V',
                    "Description":""
                },
                "PSU1_PIN": {
                    "Value": 550,
                    "LowThd": 0,
                    "HighThd": 600,
                    "Unit": 'W',
                    "Description":""
                },
            },
        }
```

#### 2.5 `fwmgrutil.py`
`fwmgrutil.py` 主要实现固件版本查看、固件升级刷新，`FRU`读写访问等功能。接口如下：

```python
class FwMgrUtil(object):
    def __init__(self):
        pass

    # 固件版本查看接口
    def get_fw_version(self):
        """
        Retrieves all firmwares' version on the device
        @return DICT, key=firmware name, value=DICT,include "version" and "description"
        """
        return 
        {
            "BIOS":{
                "Desc":"Basic Input/Output System",
                "Version":"2.0.1"
            },
            "BSP":{
                "Desc":"BSP Driver",
                "Version":"2.0.1"
            },
            "CPU_CPLD":{
                "Desc":"COMe Board CPLD",
                "Version":"2.0.2"
            },
            "CPU_IR3584_1":{
                "Desc":"CPU PVDDQ VR",
                "Version":"2.0.0"
            },
        }

    # 获取下次启动Flash以及Flash主备切换
    # BIOS
    def get_bios_current_boot_flash(self):
        return "master"/"slave"

    def get_bios_next_boot_flash(self):
        return "master"/"slave"

    def set_bios_next_boot_flash(self, flash):
        return True/False

    # Uboot
    def get_uboot_current_boot_flash(self):
        return "master"/"slave"

    def get_uboot_next_boot_flash(self):
        return "master"/"slave"

    def set_uboot_next_boot_flash(self, flash):
        return True/False

    # BMC
    def get_bmc_current_boot_flash(self):
        return "master"/"slave"

    def get_bmc_next_boot_flash(self):
        return "master"/"slave"

    def set_bmc_next_boot_flash(self, flash):
        return True/False

    # 固件更新
    def firmware_upgrade(self, fw_type, fw_path, fw_extr=None):
        """
        	@fw_type: firmware type, should be one of the strings:'bios', 'uboot', 'bmc', 'cpld', .etc.
        	@fw_path: target firmware file
        	@fw_extra OPTIONAL, extra information string, for fw_type 'BIOS'/'Uboot'/'BMC', value should be one of 'master'/'slave'/'both'
        	@return: Boolean
        """
        return True/False

    def firmware_refresh(self, fw_type, fw_path, fw_extra=None):
        return True/False

    # 获取FRU列表
    def get_frus(self):
        """
        	Retrieves all FRU names on the device
        	@return: LIST
        """
        return FRU_NAME_LIST  # ['sys', 'smb', 'pdb']

    # 获取FRU内容
    def read_fru(self, fru_name):
        """
        	Read the FRU content.
        	@fru_name: string, the name of FRU
        	@return: TUPLE,(status, output), status=True if FRU acceed success,
        			output=string of the FRU content.
        """
        return statue, content
	
    # 烧录FRU
    def program_fru(self, fru_name, fru_bin_file):
        """
        	program the FRU
        	@fru_name: the name of FRU
        	@fru_bin_file: the file of FRU raw binary.
        	@return: True if programed success.
        """
        return True/False
```

#### 2.6 `led_control.py`
`led_control.py`主要实现前面板`LEDs`和端口`LEDs`的亮灭控制，具体判断需要肉眼观察。接口如下：

```python
class LedControl(LedControlBase):
    # 设置panel sys led
    def set_sys_led(self, color):
        """
        	@color: string，should be one of "red"/'green'/'yellow' , hardware supported color
        """
        return True/False

    # 设置panel power led
    def set_power_led(self, color):
        return True/False

    # 设置panel fan led
    def set_fan_led(self, color):
        return True/False

    # 设置panel bmc led
    def set_bmc_led(self, color):
        return True/False

    # 设置port leds
    def set_port_led(self, color):
        return True/False

    # 设置 fan module led
    def set_fan_module_led(self, color):
        return True/False

    # 设置 psu module led
    def set_psu_module_led(self, color):
        return True/False

    # 设置panel location led
    def set_location_led(self, color):
        return True/False

    # 获取支持的LEDs列表
    def get_leds(self):
        return LED_NAME_LIST #['sys', 'power', 'fan', 'port']

```

#### 2.7 `mgmtport.py`
`mgmtport.py`主要实现管理网口`eth0`的访问控制，接口如下：

```python
class MgmtPortUtil(object):
    def __init__(self):
        pass

    # 获取管理口状态信息
    def get_all(self):
        """
        	Retrieves the management port running status
        	@return: DICT
        """
        return {
            "Link": True/False,
            "Firmware": "3.25",
            "Speed": "1000" or "100" or "10",
            "Duplex": "full" or "half",
            "AN": True/False,
        }

```
管理口的管理如果在`CPU`侧，`CPU`可以直接通过读写`PHY`寄存器的形式管理管理网口；如果在`BMC`侧，则需要通过`restful`接口控制。

#### 2.8 `bmcutil.py`
`bmcutil.py`主要实现`BMC`侧的访问控制以及`BMC`系统的`diag`测试。

```python
class BmcUtil(object):
    def __init__(self):
        pass

    # 执行BMC diag测试
    def run_bmc_diag(self, case=None):
        """
        	Run bmc test cases
        	@case: case name; if not specified case name, then run all cases.
        	@return: TUPLE (status, output), status=Boolean, output=string of case returned.
        """
        if not case:
            test_all()
        else:
            test_one(case)
        return (status, output)

    # 重启BMC
    def reboot_bmc(self):
        return True/False

    # 执行BMC系统下的command
    def exec_raw_cmd(self, command):
        """
        	Run command under BMC OS 
        	@command: command string,
        	@return: TUPLE (status, output), status=Boolean, output=string of case returned.
        """
        return (status, output)

```

#### 2.9 `fruidutil.py` FRU读写工具
`fruidutil.py`用于fru信息的读取和写入，重构.c 文件的output file给出的相关命令,fru文件需要先从 src 目录放到 /usr/local/bin 目录下：

``` Shell

"""
        FRU Commands:  list read write edit
        fru list                                             - list all fru
        fru list <type num>                                  - list specific fru data
        fru read <type num> <fru file>                       - store fru data to file
        fru write <type num> <fru file>                      - write fru data from file
        fru edit <type num> field <section> <index> <string> - edit FRU string

        You should move fru file to /usr/local/bin first
"""

"""
fruidutil get [NODE+NUM]
          set [NODE+NUM] -f <file>

OPTIONS:    
    option                     <get|set> fru information
    node                       <fan1|fan2|psu1|...>fru node name 
    -f                         Path of xxx.bin file
    -h                         Print use guide
"""
```

#### 2.10 `syseeprom-write.py` 
`syseeprom-write.py`用于syseeprom信息的写入：

``` Shell

'''
    write tlv eeprom to eeprom

    Usage:
        syseeprom-write.py <JSON_Path>

    e.g.:
        syseeprom-write.py ../config/common/syseeprom_test.json
'''
```


### 3 `sysdiag` 实现
`sysdiag`目录结构如下：

```shell
.
├── readme.md
├── cases
│   ├── cpu_tc
│   │   └── config.json
│   ├── memory_tc
│   │   └── config.json
│   ├── rtc_tc
│   │   └── config.json
│   ├── ssd_tc
│   ├── replaceable_eeprom_read_tc
│   │   └── config.json
│   ├── replaceable_eeprom_write_tc
│   │   └── config.json
│   ├── slot_eeprom_read_tc
│   │   └── config.json
│   ├── slot_eeprom_write_tc
│   │   └── config.json
│   └── ...
│       └── config.json
├── config
│   └── platform
│       ├── arm64-alibaba_as61_48e4t_lc-r0
│       │   ├── case_config.json
│       │   └── platform_config.json
│       ├── arm64-alibaba_as61_48x4t_lc-r0
│       ├── x86_64-h3c_s9825-64d-w1-r0
│       │   ├── case_config.json
│       │   └── platform_config.json
│       └── ...
│           ├── case_config.json
│           └── platform_config.json
└── src
    ├── cpu_tc.py
    ├── tlv_eeprom_read_tc.py
    ├── errcode.py
    ├── sysdiag.py
    ├── ...
```

其中，所有测试项的实现代码`**_tc.py`都在目录`src/`下。
`test_case.py`为定义的基础类，实现基本的测试项功能定义。
`sysdiag.py`是运行测试的主程序，主要实现输入参数解析、测试项的创建及执行、输出测试结果。

`cases/`目录是`sysdiag`的所有测试用例`**_tc`的通用配置。
`config/`目录是`sysdiag`的平台`platform`测试配置。
各配置文件说明如下：

* 测试项配置：

  * `config.json`：此文件为测试项的描述，可参考Ali设计。

    ```json
    {
        "name": "tlv-eeprom-test",               //测试项名称
        "description": "Test system TLV EEPROM", //测试项功能描述
        "type": "auto",                          //测试类型，自动化测试
        "tags": ["production", "delivery"],      //测试标签，属于生产测试和交付测试共同
        "expectation": {                         //期望的结果
            "override": "enable",
            "Serial Number": {
                "type": "string",
                "prefix": "R1241FCL",
                "subfix": "none",
                "keywords": [],
                "length": 19
            },
            "Part Number": {
                "type": "regex",
                "pattern": "R1241-F[A-Z]{4}-[0-9]{2}"
            }
        }
    }
    ```

    

* 平台配置：

  * `platform_config.json`：此文件描述了平台测试项内容，包括哪些测试项，如下

    ```json
    {
    	"test_cases": [                    //待测试项列表，说明该平台需要测试的内容
            "i2c_tc",
            "emmc_tc",
            "tlv_eeprom_tc",
            "tlv_eeprom_read_tc",
            "tlv_eeprom_write_tc",
            "replaceable_eeprom_read_tc",
            "replaceable_eeprom_write_tc",
            "slot_eeprom_read_tc",
            "slot_eeprom_write_tc"
    	]
    }
    ```
    
    * `case_config.json`：此文件描述了平台测试项的具体配置详情，比如`i2c`设备拓扑，举例如下：
  
      ```json
      {
      	"i2c":{
      		"CTRL_CPLD" : {
      			"bus" : 2,
      			"address" : "0x0d",
      			"register" : "0x0F",
      			"flag" : "rw"
      		},
      		"PORT1_CPLD" : {
      			"bus" : 3,
      			"address" : "0x30",
      			"register" : "0x0F",
      			"flag" : "rw"
      		},
      		"PORT2_CPLD" : {
      			"bus" : 4,
      			"address" : "0x31",
      			"register" : "0x0F",
      			"flag" : "rw"
      		}
      	},
      }
      ```





#### 3.1 `syslogger`日志类
定义了日志类`SysLogger`，默认打印日志到`/var/log/sysdiag.log`中，文件大小`20M`，超过之后轮转，只保留5份日志。

提供`log_dbg`、`log_info`、`log_warn`、`log_err`、`log_crit` 5种级别打印。默认只输出到日志中，可以追加参数`True`同步打印到终端。

 

#### 3.2 主类`sysdiag`

`CLI`界面使用说明如下：

```shell
$ sysdiag -h
=================================================================
  ____ ___ _____           ____            ____  _             
 |  _ \_ _|_   _|         / ___| _   _ ___|  _ \(_) __ _  __ _ 
 | |_) | |  | |    _____  \___ \| | | / __| | | | |/ _` |/ _` |
 |  __/| |  | |   |_____|  ___) | |_| \__ \ |_| | | (_| | (_| |
 |_|  |___| |_|           |____/ \__, |___/____/|_|\__,_|\__, |
                                 |___/                   |___/ 
=================================================================

         2021-01-15 17:56:33

sysdiag

    System diagnostic test utility

Usage:
    sysdiag [FUNCTION] [OPTIONS...]

FUNCTION:
    -v, --version                      Show sysdiag version
    -d, --diag [-g <tag>] [-t <type>]  Run diagnostic test, default tag is delivery
    -s, --single <casefile>            Run single test, test case file should be given
    -l, --list [-g <tag>]              Show all possible cases, default tag is delivery

OPTIONS:
    -h, --help                         Print sysdiag user guide
    -c, --chinese                      Test outputs in Chinese
    -q, --quiet                        don't print status messages to stdout")
    -t, --type                         <auto|manual|utility>    Specify type of test cases, default type is auto
    -g, --tag                          <delivery|manufacture|emc|power|pa>  Specify tag of test cases, default tag is delivery
    -i, --interval                     <min>    Set test interval when loop test, unit minute
    -b, --break                        Break when error occured under performance test
    -p, --pre handle                   pre handle

```



首先定义了`SysDiag`运行类。该类接收用户输入`sys.argv`，根据输入参数，决定下一步测试动作。

第二步，解析输入参数`parse_opts_and_args()`；支持的输入参数有`-d/-l/-s`等。

第三步，根据参数生成测试类列表`generate_test_case_list()`；这一步使用`imp.load_source()`、`getattr（）`来实现测试类的实例化，并把实例化的类加入测试列表`self.test_case_list`中。

第四步，执行测试`run()`。根据用户输入的参数，决定是执行生产测试项还是交付测试项，还是只执行单项测试。

第五步，生成测试总结果`generate_final_result()`。



#### 3.3 `test_case` 设计
`test_case.py`文件定义了`class TCBase`基类，此类主要实现对测试项配置文件`config.json`的解析，执行具体测试过程，生成测试结果。

成员函数有：

* `def __init__(self, index, case_cfg_file)`：构造函数，索引和配置文件路径，构造函数会根据传入的配置文件路径，解析出里面的`json`内容，并提供`get_tc_*()` `API`；

* **`def run_test(self, *argv):`**：需要用户实现的具体测试过程，并返回测试结果状态码；

* `def generate_result(self, code):`：已经设计好的格式化测试结果。

  ```python
  def generate_result(self, code):
      result = "Pass" if code==E.OK else "Fail"
      msg = "Test case {}: [{}] =======> {}".format(self.case_index, self.get_tc_name(), result)
      err = "ErrorCode: {}\nFail reasons: {}".format(code.name, self.fail_reason)
      return msg, err
  ```

* `def generate_result_dict(self,code)`：已经设计好的`json`格式测试结果返回函数。

特别说明，`run_test()`函数，是测试过程的具体实现，额外还需要实现对测试结果的赋值，用于测试结果输出打印。

* `self.fail_reason = [*, *, *]`，测试失败的原因列表。
* `return`：返回测试结果状态码，`E.OK`表示成功，其他值失败。



`test_case.py`文件还定义了测试类`class TestCaseCommon`，继承自`TCBase`，主要增加了对平台配置文件`case_config.json`的解析。`TestCaseCommon`是每个`test_case`实现时需要继承的基础类，用户需要根据构造函数需要传入指定参数。

* `TestCaseCommon`：

  构造函数：`__init__(self, index, module_name, logger, platform_cfg_file, case_cfg_file=None)`，构造函数会自动加载平台配置文件中的`json`内容。

  参数说明：

  * `@index`： 测试项索引，每个测试项都会分配一个ID；
  * `@module_name`： 测试项名称，**此名称和测试项配置目录名一致**；
  * `@logger`： 日志打印类，可以直接传值第4章的`logger`；
  * `@platform_cfg_file`：平台下的测试项详细配置文件，` config/platform/***/case_config.json`
  * `@case_cfg_file`： 测试项的配置文件， 缺省状态下会直接访问`cases/${module_name}/config.json`



文件命名和类名定义约束：

1. 测试项`py`文件命名，请按格式`xxxx_tc.py`，`xxxx`指定测试项名称，例如风扇测试项`fan_tc.py`；

2. 测试项中类名定义请按格式`class XXXXTC(TestCaseCommon)`，其中`TC`是`testcase`的缩写，`XXXX`是测试项文件名去掉中间连接线`_`之后的单词大写，例如风扇测试项`fan_tc.py`中类定义为`class FANTC(TestCaseCommon)`，

3. 测试类构造函数传值时，`module_name`的赋值和测试项配置目录名一致，例如风扇测试项的构造函数实现如下，其中`MODULE_NAME="fan_tc"`，`fan_tc`即是`config/cases/fan_tc/case_config.json`中的测试项配置目录名称。

   ```
   class FANTC(TestCaseCommon):
   	def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
   		MODULE_NAME = "fan_tc"
   		TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
   ```

   

举个例子，如何实现一个`i2c`测试项。

首先分析需求，`i2c`测试，需要访问验证`i2c bus`下的从设备（涉及`cpld`、`sfp eerpom`、`fru`、`rtc`等）是否工作正常，对于`cpld`可以通过读写测试寄存器验证，其他设备可以读测试。

基于需要，`i2c`测试类继承`TestCaseCommon`，并需要实现接口`run_test()`。

并且，`i2c`测试还需要该平台下的`i2c`拓扑，故在`config/platform/*/case_config.json`中增加`i2c`配置如下：

```json
{
	"i2c":{
		"CTRL_CPLD" : {
			"bus" : 2,
			"address" : "0x0d",
			"register" : "0x0F",
			"flag" : "rw"
		},
		"PORT1_CPLD" : {
			"bus" : 3,
			"address" : "0x30",
			"register" : "0x0F",
			"flag" : "rw"
		},
		"PORT2_CPLD" : {
			"bus" : 4,
			"address" : "0x31",
			"register" : "0x0F",
			"flag" : "rw"
		}
	}
}
```

定义`i2c`测试类如下：

```python
class I2CTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "i2c_tc" # this param specified the case config dirictory
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        try:
            if self.platform_cfg_json and 'i2c' in self.platform_cfg_json.keys():
                self.i2c_devices = self.platform_cfg_json['i2c_topo']
        except Exception as e:
            self.logger.log_err(str(e))
            self.i2c_devices = None
```

并实现`i2c rw`函数如下，遍历访问设备，对于读写失败的设备，记录失败原因到`fail_reason`列表中。

```python
def i2c_device_rw_test(self, also_print_console=False):
    '''
        i2c device read/write test
    '''
```

`run_test`函数中调用`i2c_device_rw_test`即可；



#### 3.4 错误码定义
文件`errorcode.py`，错误码定义采用枚举定义的方法，定义常见的错误类型。

如下：
```python
class E(Enum):
    OK = 0           # pass
    EFAIL = 1        # fail
    EUSB1000 = 1000  # usb not found
    EUSB1001 = 1001  # usb write error
    ...
```
测试项`xxxx_tc.py`直接引入该模块。

#### 3.5 测试项`xxxx_tc.py`实现参考

```python
from test_case import TestCaseCommon
from errcode import E

class XXXXTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "xxxx_tc"  # this param specified the case config dirictory
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
    
    def test_a(self):
        if fail:
            self.fail_reason.append("usb not found")
            return E.EUSB1000
        else:
            return E.OK
    
    def test_b(self):
        if fail:
            self.fail_reason.append("usb write error")
            return E.EUSB1001
        else:
            return E.OK
    
    # 必须实现的函数
    def run_test(self, *argv):
        ret = self.test_a()
        if ret != E.OK:
            return ret.value
        ret = self.test_b()
        return ret.value

```
