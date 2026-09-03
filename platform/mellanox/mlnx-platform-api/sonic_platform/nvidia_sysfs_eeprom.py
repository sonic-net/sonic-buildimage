#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""EEPROM read/write transport for the NVIDIA sx_core sysfs layout.

Unlike optoe, which exposes a module's EEPROM as a single flat file, sx_core
exposes one file per CMIS page under a directory tree::

    /sys/module/sx_core/asic0/module<N>/[bank<B>/]eeprom/pages/
        0/i2c-0x50/data   lower + upper page 00h (256 bytes)
        0/i2c-0x51/data   A2h alias, SFF8472 only (256 bytes)
        <page>/data       one 128-byte file per upper page

A linear CMIS offset therefore has to be translated into a (page number, page
file, offset within that file) triple before any I/O. That translation is the
same for every sx_core device; only the presence of the ``bank<B>`` segment
differs, which is what splits the mixin into an unbanked and a banked flavour.

This module also owns the sysfs path hierarchy every sx_core consumer needs,
so that the module index is rendered in exactly one place.
"""

import os
from abc import ABC, abstractmethod

from sonic_py_common.logger import Logger
from sonic_platform_base.sonic_xcvr.eeprom_rw import EepromReadWriteMixin
from sonic_platform_base.sonic_xcvr.mem_maps.public.cmis import CMIS_ARCH_PAGES

logger = Logger()

SX_CORE_ASIC_ROOT = '/sys/module/sx_core/asic0'

SFP_PAGE0_PATH = '0/i2c-0x50/data'
SFP_A2H_PAGE0_PATH = '0/i2c-0x51/data'

SFP_PAGE_SIZE = 256          # page size of page 00h, lower plus upper memory
SFP_UPPER_PAGE_OFFSET = 128  # page size of every other page

# Stride between banks in the linear space, matching CmisPage.linear_offset:
# every bank is a full CMIS_ARCH_PAGES block, so bank B starts at
# B * CMIS_ARCH_PAGES * SFP_UPPER_PAGE_OFFSET. Do not widen this to account for
# page 00h being 256 bytes; the mem maps address banks on exactly this stride, and
# changing it shifts every banked page by one.
CMIS_BYTES_PER_BANK = CMIS_ARCH_PAGES * SFP_UPPER_PAGE_OFFSET  # 256 * 128 = 32KB

# Flags the A2h alias file, which is not a real CMIS page number.
A2H_ALIAS_PAGE_NUM = -1


def module_root(sdk_index):
    """Root of one module's sysfs tree.
    'SX_CORE_ASIC_ROOT'/module<N>
    """
    return os.path.join(SX_CORE_ASIC_ROOT, f'module{sdk_index}')


def bank_root(sdk_index, bank):
    """Root of one bank's sysfs tree under its module.
    'SX_CORE_ASIC_ROOT'/module<N>/bank<B>
    """
    return os.path.join(module_root(sdk_index), f'bank{bank}')


def module_attr_path(sdk_index, name):
    """Path of a single module-level sysfs attribute, for example 'present'.
    'SX_CORE_ASIC_ROOT'/module<N>/<name>
    """
    return os.path.join(module_root(sdk_index), name)


def module_pages_root(sdk_index):
    """Root of the per-page EEPROM tree of a module that has no banks.
    'SX_CORE_ASIC_ROOT'/module<N>/eeprom/pages
    """
    return os.path.join(module_root(sdk_index), 'eeprom', 'pages')


def bank_pages_root(sdk_index, bank):
    """Root of the per-page EEPROM tree of one bank of a banked module.
    'SX_CORE_ASIC_ROOT'/module<N>/bank<B>/eeprom/pages
    """
    return os.path.join(bank_root(sdk_index, bank), 'eeprom', 'pages')


class NvidiaSysfsEeprom(EepromReadWriteMixin, ABC):
    """Per-page sysfs EEPROM access for sx_core backed devices.

    Subclasses supply the device identity (`get_sdk_index`) and the shape of its
    EEPROM tree (`_get_eeprom_path`); everything above that is layout
    translation shared by every sx_core device, CPO or not.

    Hosts that can tell more about the part they carry refine two hooks:
    `_uses_a2h_alias` and `_is_write_protected`. Both need EEPROM content or
    module state to answer, which is knowledge this layer deliberately lacks.
    """

    @abstractmethod
    def get_sdk_index(self):
        """Module index used to render the sysfs path."""
        raise NotImplementedError

    @abstractmethod
    def _get_eeprom_path(self, bank_id=0):
        """Directory holding this device's page files, for the given bank.

        The page number is not part of it: pages are files inside the returned
        directory, and callers join the one they want onto it.
        """
        raise NotImplementedError

    @abstractmethod
    def _split_bank_offset(self, overall_offset):
        """Split a linear offset into the bank it names and the offset within it.

        How the linear space divides into banks is precisely what separates the
        two flavours, so neither the stride nor the absence of one belongs in the
        shared translation below.
        """
        raise NotImplementedError

    def module_attr_path(self, name):
        return module_attr_path(self.get_sdk_index(), name)

    def _uses_a2h_alias(self, eeprom_root):
        """Whether bytes 256-511 alias the A2h file rather than upper page 01h.

        Only SFF8472 parts do. Deciding that requires reading the EEPROM, so
        the default is off and hosts that can read it override this.
        """
        return False

    def _is_write_protected(self, page, page_offset, num_bytes):
        """Whether the given span must not be written."""
        return False

    def _get_page_and_page_offset(self, overall_offset):
        """Translate a linear EEPROM offset into a concrete page file.

        Args:
            overall_offset (int): linear offset across banks and pages

        Returns:
            tuple: (page_num, page file path, offset within that file), or
                   (None, None, None) if the module's sysfs tree is absent
        """
        bank_id, within_bank_offset = self._split_bank_offset(overall_offset)

        # Lower memory and page 00h are not banked, so they always render at bank 0.
        page0_path = self._get_eeprom_path(bank_id=0)
        if not os.path.exists(page0_path):
            logger.log_error(f'EEPROM file path for sfp {self.get_sdk_index()} does not exist')
            return None, None, None

        if within_bank_offset < SFP_PAGE_SIZE:
            return 0, os.path.join(page0_path, SFP_PAGE0_PATH), within_bank_offset

        if self._uses_a2h_alias(page0_path):
            page1h_start = SFP_PAGE_SIZE * 2
            if within_bank_offset < page1h_start:
                return (A2H_ALIAS_PAGE_NUM,
                        os.path.join(page0_path, SFP_A2H_PAGE0_PATH),
                        within_bank_offset - SFP_PAGE_SIZE)
        else:
            page1h_start = SFP_PAGE_SIZE

        page_num = (within_bank_offset - page1h_start) // SFP_UPPER_PAGE_OFFSET + 1
        offset = (within_bank_offset - page1h_start) % SFP_UPPER_PAGE_OFFSET

        upper_page_path = self._get_eeprom_path(bank_id=bank_id)

        return page_num, os.path.join(upper_page_path, f'{page_num}/data'), offset

    def read_eeprom(self, offset, num_bytes):
        """
        Read eeprom specfic bytes beginning from a random offset with size as num_bytes

        Returns:
            bytearray, if raw sequence of bytes are read correctly from the offset of size num_bytes
            None, if the read_eeprom fails
        """
        return self._read_eeprom(offset, num_bytes)

    def _read_eeprom(self, offset, num_bytes, log_on_error=True):
        """Read num_bytes starting at a linear offset, spanning pages as needed.

        Args:
            offset (int): read offset
            num_bytes (int): read size
            log_on_error (bool, optional): whether log error when exception occurs. Defaults to True.

        Returns:
            bytearray: the content of EEPROM, or None on failure
        """
        sdk_index = self.get_sdk_index()
        result = bytearray(0)
        while num_bytes > 0:
            _, page, page_offset = self._get_page_and_page_offset(offset)
            if not page:
                return None

            try:
                with open(page, mode='rb', buffering=0) as f:
                    f.seek(page_offset)
                    content = f.read(num_bytes)
                    if not result:
                        result = content
                    else:
                        result += content
                    read_length = len(content)
                    if read_length == 0:
                        logger.log_error(f'SFP {sdk_index}: EEPROM page {page} is empty, no data retrieved')
                        return None
                    num_bytes -= read_length
                    if num_bytes > 0:
                        page_size = f.seek(0, os.SEEK_END)
                        if page_offset + read_length == page_size:
                            offset += read_length
                        else:
                            # Indicate read finished
                            num_bytes = 0
                    logger.log_debug(f'read EEPROM sfp={sdk_index}, page={page}, page_offset={page_offset}, '
                                     f'size={read_length}, data={content}')
            except (OSError, IOError) as e:
                if log_on_error:
                    logger.log_warning(f'Failed to read sfp={sdk_index} EEPROM page={page}, page_offset={page_offset}, '
                                       f'size={num_bytes}, offset={offset}, error = {e}')
                return None

        return bytearray(result)

    def write_eeprom(self, offset, num_bytes, write_buffer):
        """Write num_bytes at a linear offset, spanning pages as needed.

        Args:
            offset (int): write offset
            num_bytes (int): write size
            write_buffer (bytearray): bytes to write

        Returns:
            bool: True if the whole write succeeded
        """
        if num_bytes != len(write_buffer):
            logger.log_error('Error mismatch between buffer length and number of bytes to be written')
            return False

        sdk_index = self.get_sdk_index()
        while num_bytes > 0:
            page_num, page, page_offset = self._get_page_and_page_offset(offset)
            if not page:
                return False

            try:
                if self._is_write_protected(page_num, page_offset, num_bytes):
                    # write limited eeprom is not supported
                    raise IOError('write limited bytes')
                with open(page, mode='r+b', buffering=0) as f:
                    f.seek(page_offset)
                    ret = f.write(write_buffer[0:num_bytes])
                    written_buffer = write_buffer[0:ret]
                    if ret != num_bytes:
                        page_size = f.seek(0, os.SEEK_END)
                        if page_offset + ret == page_size:
                            # Move to next page
                            write_buffer = write_buffer[ret:num_bytes]
                            offset += ret
                        else:
                            raise IOError(f'write return code = {ret}')
                    num_bytes -= ret
                    logger.log_debug(f'write EEPROM sfp={sdk_index}, page={page}, page_offset={page_offset}, '
                                     f'size={ret}, left={num_bytes}, data={written_buffer}')
            except (OSError, IOError) as e:
                data = ''.join('{:02x}'.format(x) for x in write_buffer)
                logger.log_error(f'Failed to write EEPROM data sfp={sdk_index} EEPROM page={page}, '
                                 f'page_offset={page_offset}, size={num_bytes}, offset={offset}, '
                                 f'data = {data}, error = {e}')
                return False
        return True


class NvidiaSysfsEepromUnbanked(NvidiaSysfsEeprom):
    """sx_core devices whose EEPROM tree has no 'bank<B>' segment.

    bank_id is accepted for symmetry with the banked flavour and ignored.
    """

    def _get_eeprom_path(self, bank_id=0):
        return module_pages_root(self.get_sdk_index())

    def _split_bank_offset(self, overall_offset):
        """A flat device has one bank, so the offset is already within it.

        Reducing modulo CMIS_BYTES_PER_BANK here would put page FFh, whose upper
        half starts at exactly that stride, back at page 00h.
        """
        return 0, overall_offset


class NvidiaSysfsEepromBanked(NvidiaSysfsEeprom):
    """sx_core devices whose EEPROM tree is split per bank.

    bank_id is the bank implied by the linear offset being translated, which
    differs from the device's own bank when a read crosses a bank boundary.
    """

    def _get_eeprom_path(self, bank_id=0):
        return bank_pages_root(self.get_sdk_index(), bank_id)

    def _split_bank_offset(self, overall_offset):
        return overall_offset // CMIS_BYTES_PER_BANK, overall_offset % CMIS_BYTES_PER_BANK
