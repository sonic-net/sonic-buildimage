import os
import ast
import imp
import yaml
import subprocess

from sonic_py_common import device_info


class Common:

    DEVICE_PATH = '/usr/share/sonic/device/'
    PMON_PLATFORM_PATH = '/usr/share/sonic/platform/'
    CONFIG_DIR = 'sonic_platform_config'

    OUTPUT_SOURCE_IPMI = 'ipmitool'
    OUTPUT_SOURCE_GIVEN_LIST = 'value_list'
    OUTPUT_SOURCE_GIVEN_VALUE = 'value'
    OUTPUT_SOURCE_GIVEN_CLASS = 'class'
    OUTPUT_SOURCE_SYSFS = 'sysfs_value'
    OUTPUT_SOURCE_FUNC = 'function'
    OUTPUT_SOURCE_GIVEN_TXT_FILE = 'txt_file'
    OUTPUT_SOURCE_GIVEN_VER_HEX_FILE = 'hex_version_file'
    OUTPUT_SOURCE_GIVEN_VER_HEX_ADDR = 'hex_version_getreg'

    SET_METHOD_IPMI = 'ipmitool'
    NULL_VAL = 'N/A'
    HOST_CHK_CMD = ["docker"]
    REF_KEY = '$ref:'

    def __init__(self, conf=None):
        self._main_conf = conf
        (self.platform, self.hwsku) = device_info.get_platform_and_hwsku()

    def _run_command(self, command):
        status = False
        output = ""
        try:
            p = subprocess.Popen(
                command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            raw_data, err = p.communicate()
            if err == '':
                status, output = True, raw_data.strip()
        except Exception:
            pass
        return status, output

    def _clean_input(self, input, config):
        cleaned_input = input

        ai = config.get('avaliable_input')
        if ai and input not in ai:
            return None

        input_translator = config.get('input_translator')
        if type(input_translator) is dict:
            cleaned_input = input_translator.get(input)

        elif type(input_translator) is str:
            cleaned_input = ast.literal_eval(input_translator.format(input))

        return cleaned_input

    def _clean_output(self, index, output, config):
        output_translator = config.get('output_translator')

        if type(output_translator) is dict:
            output = output_translator.get(output)
        elif type(output_translator) is str:
            output = ast.literal_eval(output_translator.format(output))
        elif type(output_translator) is list:
            output = ast.literal_eval(output_translator[index].format(output))

        return output

    def _ipmi_get(self, index, config):
        argument = config.get('argument')
        cmd = config['command'].format(
            config['argument'][index]) if argument else config['command']
        status, output = self._run_command(cmd)
        return output if status else None

    def _sysfs_read(self, index, config):
        sysfs_path = config.get('sysfs_path')
        argument = config.get('argument', '')

        if self.REF_KEY in argument:
            argument = self._main_conf[argument.split(":")[1]]

        if type(argument) is list:
            sysfs_path = sysfs_path.format(argument[index])

        content = ""
        try:
            content = open(sysfs_path)
            content = content.readline().rstrip()
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        return content

    def _sysfs_write(self, index, config, input):
        sysfs_path = config.get('sysfs_path')
        argument = config.get('argument', '')

        if self.REF_KEY in argument:
            argument = self._main_conf[argument.split(":")[1]]

        if type(argument) is list:
            sysfs_path = sysfs_path.format(argument[index])

        write_offset = int(config.get('write_offset', 0))
        output = ""
        try:
            open_file = open(sysfs_path, "r+")
            open_file.seek(write_offset)
            open_file.write(input)
            open_file.close()
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False, output
        return True, output

    def _ipmi_set(self, index, config, input):
        arg = config['argument'][index].format(input)
        return self._run_command(config['command'].format(arg))

    def _hex_ver_decode(self, hver, num_of_bits, num_of_points):
        ver_list = []
        c_bit = 0
        bin_val = bin(int(hver, 16))[2:].zfill(num_of_bits)
        bit_split = num_of_bits / (num_of_points + 1)
        for x in range(0, num_of_points+1):
            split_bin = bin_val[c_bit:c_bit+bit_split]
            ver_list.append(str(int(split_bin, 2)))
            c_bit += bit_split
        return '.'.join(ver_list)

    def _get_class(self, config):
        """
        Retreives value of expected attribute
        Returns:
            A value of the attribute of object
        """
        path = config['host_path'] if self.is_host() else config['pmon_path']
        module = imp.load_source(config['class'], path)
        class_ = getattr(module, config['class'])
        return class_

    def get_reg(self, path, reg_addr):
        cmd = "echo {1} > {0}; cat {0}".format(path, reg_addr)
        status, output = self._run_command(cmd)
        return output if status else None

    def read_txt_file(self, path):
        with open(path, 'r') as f:
            output = f.readline()
        return output.strip('\n')

    def write_txt_file(self, file_path, value):
        try:
            with open(file_path, 'w') as fd:
                fd.write(str(value))
        except Exception:
            return False
        return True

    def is_host(self):
        try:
            subprocess.call(self.HOST_CHK_CMD, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except FileNotFoundError:
            return False
        return True

    def load_json_file(self, path):
        """
        Retrieves the json object from json file path

        Returns:
            A json object
        """
        with open(path, 'r') as f:
            json_data = yaml.safe_load(f)

        return json_data

    def get_config_path(self, config_name):
        """
        Retrieves the path to platform api config directory

        Args:
            config_name: A string containing the name of config file.

        Returns:
            A string containing the path to json file
        """
        return os.path.join(self.DEVICE_PATH, self.platform, self.CONFIG_DIR, config_name) if self.is_host() else os.path.join(self.PMON_PLATFORM_PATH, self.CONFIG_DIR, config_name)

    def get_output(self, index, config, default):
        """
        Retrieves the output for each function base on config

        Args:
            index: An integer containing the index of device.
            config: A dict object containing the configuration of specified function.
            default: A string containing the default output of specified function.

        Returns:
            A string containing the output of specified function in config
        """
        output_source = config.get('output_source')

        if output_source == self.OUTPUT_SOURCE_IPMI:
            output = self._ipmi_get(index, config)

        elif output_source == self.OUTPUT_SOURCE_GIVEN_VALUE:
            output = config["value"]

        elif output_source == self.OUTPUT_SOURCE_GIVEN_CLASS:
            output = self._get_class(config)

        elif output_source == self.OUTPUT_SOURCE_GIVEN_LIST:
            output = config["value_list"][index]

        elif output_source == self.OUTPUT_SOURCE_SYSFS:
            output = self._sysfs_read(index, config)

        elif output_source == self.OUTPUT_SOURCE_FUNC:
            func_conf = self._main_conf[config['function'][index]]
            output = self.get_output(index, func_conf, default)

        elif output_source == self.OUTPUT_SOURCE_GIVEN_TXT_FILE:
            path = config.get('path')
            output = self.read_txt_file(path)

        elif output_source == self.OUTPUT_SOURCE_GIVEN_VER_HEX_FILE:
            path = config.get('path')
            hex_ver = self.read_txt_file(path)
            output = self._hex_ver_decode(
                hex_ver, config['num_of_bits'], config['num_of_points'])

        elif output_source == self.OUTPUT_SOURCE_GIVEN_VER_HEX_ADDR:
            path = config.get('path')
            addr = config.get('reg_addr')
            hex_ver = self.get_reg(path, addr)
            output = self._hex_ver_decode(
                hex_ver, config['num_of_bits'], config['num_of_points'])

        else:
            output = default

        return self._clean_output(index, output, config) or default

    def set_output(self, index, input, config):
        """
        Sets the output of specified function on config

        Args:
            config: A dict object containing the configuration of specified function.
            index: An integer containing the index of device.
            input: A string containing the input of specified function.

        Returns:
            bool: True if set function is successfully, False if not
        """
        cleaned_input = self._clean_input(input, config)
        if not cleaned_input:
            return False

        set_method = config.get('set_method')
        if set_method == self.SET_METHOD_IPMI:
            output = self._ipmi_set(index, config, cleaned_input)[0]
        elif set_method == self.OUTPUT_SOURCE_SYSFS:
            output = self._sysfs_write(index, config, cleaned_input)[0]
        else:
            output = False

        return output

    def get_event(self, timeout, config, sfp_list):
        """
        Returns a nested dictionary containing all devices which have
        experienced a change at chassis level

        """
        event_class = self._get_class(config)
        return event_class(sfp_list).get_event(timeout)
