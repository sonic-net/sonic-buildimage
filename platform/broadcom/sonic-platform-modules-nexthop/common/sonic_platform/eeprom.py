# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

try:
    from sonic_platform_pddf_base.pddf_eeprom import PddfEeprom
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Eeprom(PddfEeprom):

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        PddfEeprom.__init__(self, pddf_data, pddf_plugin_data)

        if not hasattr(self, "eeprom_data") or self.eeprom_data == "N/A":
            return

        self.eeprom_tlv_dict = {}
        self._parse_raw_buffer(self.eeprom_data)

    def _parse_raw_buffer(self, buf):
        if not self.is_valid_tlvinfo_header(buf):
            return

        total_len = ((buf[9]) << 8) | (buf[10])
        idx = self._TLV_INFO_HDR_LEN
        end = self._TLV_INFO_HDR_LEN + total_len

        while idx + 2 <= len(buf) and idx < end:
            t_code = buf[idx]
            length = buf[idx + 1]
            v_start = idx + 2
            v_end = v_start + length

            if v_end > len(buf):
                break

            code_str = "0x%02X" % t_code
            val_bytes = buf[v_start:v_end]

            if t_code == self._TLV_CODE_VENDOR_EXT:
                if code_str not in self.eeprom_tlv_dict:
                    self.eeprom_tlv_dict[code_str] = []

                formatted_hex = " ".join(["0x%02X" % b for b in buf[v_start:v_end]])
                self.eeprom_tlv_dict[code_str].append(formatted_hex)
            else:
                _, decoded = self.decoder(None, buf[idx:v_end])
                self.eeprom_tlv_dict[code_str] = decoded

            if t_code == self._TLV_CODE_CRC_32:
                break

            idx = v_end
