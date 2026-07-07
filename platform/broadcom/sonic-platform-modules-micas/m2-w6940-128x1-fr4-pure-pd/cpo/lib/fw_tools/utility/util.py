# import numpy as np
import re
from typing import Iterable
import struct
import math
import os
import time
import datetime

LITTLE_ENDIAN = 0
BIG_ENDIAN = 1
class UtilObj(object):
    def __init__(self):
        return

    @staticmethod
    def uint8_to_uint16(data_bytes, data_type=LITTLE_ENDIAN):
        if data_type == LITTLE_ENDIAN:
            return (data_bytes[1] << 8) + data_bytes[0]
        elif data_type ==BIG_ENDIAN:
            return (data_bytes[0] << 8) + data_bytes[1]

    @staticmethod
    def uint8_to_uint32(data_bytes, data_type=LITTLE_ENDIAN):
        if data_type == LITTLE_ENDIAN:
            return (data_bytes[3] << 24) + (data_bytes[2] << 16) + (data_bytes[1] << 8) + data_bytes[0]
        elif data_type ==BIG_ENDIAN:
            return (data_bytes[0] << 24) + (data_bytes[1] << 16) + (data_bytes[2] << 8) + data_bytes[3]

    @staticmethod
    def get_str_line_checksum(text_line):
        text_data = [text_line[x:x+2] for x in range(0, len(text_line), 2)]
        cs = 0
        for item in text_data:
            cs += int(item, 16)
        cs = ~(cs & 0xFF) + 1
        return "%02X" % (cs & 0xFF)

    @staticmethod
    def get_8bit_check_sum(data_buffer):
        sum_value = 0
        for x in data_buffer:
            sum_value += x
        sum_value &= 0xFF
        return sum_value

    @staticmethod
    def get_8bit_xor_check_sum(data_buffer):
        sum_value = 0
        for x in data_buffer:
            sum_value += x
        sum_value &= 0xFF
        sum_value ^= 0xFF
        sum_value &= 0xFF
        return sum_value

    @staticmethod
    def is_eol(c):
        """is_eol() checks if the character is end of line.
        Args:
            (str) c
        Returns:
            True: end of line
            False: not end of line"""
        return (c == 10) or (c == 13)

    @classmethod
    def check_sum_calculate(cls, hex_file):
        sum_value = 0
        for c in iter(lambda: hex_file.read(1), ''):
            if not cls.is_eol(ord(c)):
                sum_value += ord(c)

        sum_value &= 0xFFFF
        return sum_value

    @classmethod
    def calculate_checksum(cls, hex_file):
        sum_value = 0

        with open(hex_file) as f:
            for c in iter(lambda: f.read(1), ''):
                if not cls.is_eol(ord(c)):
                    sum_value += ord(c)

        f.close()
        sum_value &= 0xFFFF

        return sum_value

    @staticmethod
    def calculate_crc(init_crc, input_data):
        polynomial = 0x04C11DB7

        crc_result = init_crc ^ input_data

        for i in range(32):
            if crc_result & 0x80000000:
                crc_result = (crc_result << 1) ^ polynomial
            else:
                crc_result = (crc_result << 1)

        return crc_result & 0xFFFFFFFF

    @classmethod
    def calculate_buffer_crc(cls, data_buffer, init_crc=0xFFFFFFFF):
        crc_result = init_crc

        for i in range(0, len(data_buffer), 4):
            crc_data = (data_buffer[i] << 24) + (data_buffer[i + 1] << 16) + \
                    (data_buffer[i + 2] << 8) + data_buffer[i + 3]
            crc_result = cls.calculate_crc(crc_result, crc_data)
        return crc_result & 0xFFFFFFFF

    @classmethod
    def check_sum_valid(cls, hex_file, check_sum):
        """check_sum_valid() checks the hex file to see if it has the correct checksum
        Args:
            (File) hex_file
            (int) check_sum
        Returns:
            (bool) checksum success/fail"""
        sum_value = 0

        with open(hex_file) as f:

            for c in iter(lambda: f.read(1), ''):
                if not cls.is_eol(ord(c)):
                    sum_value += ord(c)
        f.close()

        sum_value &= 0xFFFF

        return sum_value == check_sum

    @staticmethod
    def twos_comp(val, bits):
        """compute the 2's complement of int value val"""
        if (val & (1 << (bits - 1))) != 0:
            val = val - (1 << bits)
        return val

    @staticmethod
    def read_hex_file_simple(filename):
        hex_file = open(filename, 'r')
        data_buffer = []
        idx = 0
        file_line = hex_file.readline()
        while file_line != '':
            data_length = int(file_line[1:3], 16)
            data_type = int(file_line[7:9], 16)
            if data_type == 0x00:
                for n in range(data_length):
                    data_buffer.append(int(file_line[(9 + 2 * n):(11 + 2 * n)], 16))
                    idx += 1
            file_line = hex_file.readline()
        hex_file.close()
        return data_buffer

    @staticmethod
    def read_hex_file(filename, byte_size=0x8000):
        hex_file = open(filename, 'r')
        data_buffer = [[0 for i in range(2)] for j in range(byte_size)]
        data_buffer_index = 0

        file_line = hex_file.readline()
        while file_line != '':
            data_length = int(file_line[1:3], 16)
            data_type = int(file_line[7:9], 16)
            if data_type == 0x00:
                eeprom_index = int(file_line[3:7], 16)
                for i in range(eeprom_index, eeprom_index + data_length):
                    data_buffer[i][0] = 1
                    data_buffer[i][1] = int(file_line[(9 + 2 * (i-eeprom_index)):(11 + 2 * (i-eeprom_index))], 16)
                data_buffer_index += data_length

            if data_buffer_index > (byte_size + 1):
                print('Total number of data bytes has exceeded %d.' % byte_size)
                return False, None

            file_line = hex_file.readline()
        hex_file.close()
        return True, data_buffer

    @staticmethod
    def extract_hex_one_page_spaced_data(hex_file, data_buffer):
        """extract_one_page_data() extracts 256 bytes of data bytes from the hex file.
        Args:
            (File) hex_file
            (Array) data_buffer
        Returns:
            (int) status"""

        page_size = len(data_buffer)
        line_size = 32
        if page_size % line_size != 0:
            return False
        line_num = int(page_size / line_size)

        for i in range(line_num):
            file_line = hex_file.readline()
            for j in range(line_size):
                data_buffer[int(i * line_size + j)] = int(file_line[int(3 * j):int(3 * j + 2)], 16)

        return True

    @staticmethod
    def extract_intel_hex_one_page_data(hex_file, data_buffer):
        """extract_one_page_data() extracts 256 bytes of data bytes from the hex file.
        Args:
            (File) hex_file
            (Array) data_buffer
        Returns:
            (int) status"""
        byte_number = 0
        page_size = len(data_buffer)

        while byte_number < page_size:                                  # Extract one page
            file_line = hex_file.readline()                             # Read one line
            if file_line == '':                                         # End of file
                if byte_number < page_size:                             # Data buffer is not filled
                    for i in range(byte_number, page_size):             # Fill the empty bytes with 0xFF
                        data_buffer[i] = 0xFF
                return True
            if int(file_line[7:9], 16) == 0x00:                         # If the hex line contains data
                length = int(file_line[1:3], 16)                        # Calculate number of bytes in the hex line
                for i in range(length):                                 # Store each bytes into the data buffer
                    data_buffer[byte_number + i] = int(file_line[(9 + 2 * i): 11 + 2 * i], 16)
                byte_number += length
        return True

    @staticmethod
    def write_to_file(hex_file, data_buffer):
        if len(data_buffer) % 16 != 0:
            return False

        with open(hex_file, 'w') as f:
            f.write(':020000040000FA\n')
            start_address = 0x0000
            for i in range(0, len(data_buffer), 16):
                data_line = ':10%04X00' % start_address
                chk_sum = 0x10
                chk_sum += (start_address >> 8) & 0xFF
                chk_sum += start_address & 0xFF

                for j in range(16):
                    data_line = data_line + '%02X' % data_buffer[i + j]
                    chk_sum += int(data_buffer[i + j])
                chk_sum = ~(chk_sum & 0xFF) + 1
                data_line = data_line + '%02X\n' % (chk_sum & 0xFF)
                f.write(data_line)
                start_address += 0x10

            f.write(':00000001FF\n')
        f.close()
        return True

    @staticmethod
    def write_to_file_spaced_data(hex_file, data_buffer):
        if len(data_buffer) % 32 != 0:
            return False

        with open(hex_file, 'w') as f:
            for i in range(0, len(data_buffer), 32):
                data_line = " ".join(map("{:02x}".format, data_buffer[i:i+32])) + ' \n'
                f.write(data_line)

        f.close()
        return True

    @staticmethod
    def print_data_table(col_header, row_header, data_buffer, spacing=8):
        arr = [['-' for i in range(len(row_header))] for j in range(len(col_header))]
        data_array = [None] * (len(row_header) * len(col_header))
        for i in range(len(data_buffer)):
            if data_buffer[i] is not None:
                data_array[i] = data_buffer[i]
        for i in range(len(col_header)):
            for j in range(len(row_header)):
                if data_array[i * len(row_header) + j] is not None:
                    arr[i][j] = str(data_array[i * len(row_header) + j])
        row_format = "{:^{spacing}}" * (len(row_header) + 1)
        print(row_format.format("", *row_header, spacing=spacing))
        for col, row in zip(col_header, arr):
            print(row_format.format(col, *row, spacing=spacing))

    @staticmethod
    def parse_int_args(my_str: str, reverse=False):
        if reverse not in [True, False]:
            raise ValueError('reverse arg must be bool')

        my_str = my_str.strip()
        try:
            temp = [(lambda sub: range(sub[0], sub[-1] + 1))(list(map(int, ele.split('-')))) for ele in my_str.split(', ')]
        except:
            temp = [(lambda sub: range(sub[0], sub[-1] + 1))(list(map(int, ele.split('-')))) for ele in my_str.split(',')]
        res = [b for a in temp for b in a]
        # sort the
        res = sorted(res, reverse=reverse)
        return res

    @staticmethod
    def gen_port_str(*ports, prefix='', verbose=True, **kwports) -> str:
        """
        This function is to generate the ports string depends on the input
        :param *ports: ports without specify the keywords, *ports can be single int, list and tuple of ints for
        :              multiple channels
        :param *kwports: ports with specify the keywords, *kwports can be single int, list or tuple
        :                special for keyword with "start" and "end", for example, start = 10, end = 20 means 10-20
        :prefix is added so that for ports like cd0,cd1,cd3 etc, input [0,1,2,3], and later add the common prefix later.
        :return the return value will be a string in format '1-2,4,10-20,25'
        """
        pc_ports = set()
        for item in ports:
            if type(item) is int:
                pc_ports.add(item)
            elif isinstance(item, Iterable):
                pc_ports.update(item)
            else:
                raise ValueError(f"Parameter '{item}' error, parameter must be int or list with int")

        specified_pair_start = []
        specified_pair_end = []
        for key, item in kwports.items():
            if 'start' in key.lower():
                specified_pair_start.append(item)
            elif 'end' in key.lower() or 'stop' in key.lower():
                specified_pair_end.append(item)

            elif type(item) is int:
                pc_ports.add(item)
            elif isinstance(item, Iterable):
                pc_ports.update(item)
            else:
                raise ValueError(f"Parameter '{key} = {item}' error, parameter must be int or list with int")

        # if len(specified_pair_start) > 0 and len(specified_pair_end) > 0:
        if len(specified_pair_start) == len(specified_pair_end):
            for _start, _end in zip(specified_pair_start, specified_pair_end):
                pc_ports.update(range(_start, _end + 1))
        else:
            raise ValueError("start and end must be used in pair")

        pc_ports_list = list(pc_ports)
        pc_ports_list.sort()

        # merge the sections
        pc_ports_sections = []
        start = pc_ports_list[0]
        prev = pc_ports_list[0]
        for _idx, item in enumerate(pc_ports_list):

            if item - prev > 1:
                pc_ports_sections.append([start, prev])
                start = item
            if _idx == len(pc_ports_list) - 1:
                pc_ports_sections.append([start, item])
            prev = item
        # logger.debug(f"All Ports: {pc_ports_list}")

        pc_ports_section_strs = []
        for section in pc_ports_sections:
            if section[0] == section[1]:
                pc_ports_section_strs.append(str(section[0]))
            else:
                pc_ports_section_strs.append(f"{section[0]}-{section[1]}")
        _all_ports_str = ','.join(pc_ports_section_strs)

        # dealing with add prefix
        all_ports_str = prefix
        all_ports_str += _all_ports_str
        all_ports_str = all_ports_str.replace('-', f'-{prefix}')
        all_ports_str = all_ports_str.replace(',', f',{prefix}')

        if verbose:
            print(f"Generated ASIC port string: {all_ports_str}")
        return all_ports_str

    @staticmethod
    def gen_sorted_port_lst(*ports, verbose=True):
        if isinstance(ports, int):
            ports = [ports]

        _port_str = UtilObj.gen_port_str(*ports, verbose=verbose)
        _lc_ports_id = UtilObj.parse_int_args(_port_str)

        _rtn = sorted(list(set(_lc_ports_id)))

        return _rtn
