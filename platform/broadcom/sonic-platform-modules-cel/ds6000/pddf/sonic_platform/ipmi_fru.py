#!/usr/bin/env python3
# ---------------------------------------------------------------------------
# ipmi_fru_parser.py — Universal IPMI Platform Management FRU Information parser
#                      (Structured Dict Version - Supporting DS6000/DS6001)
# ---------------------------------------------------------------------------

import sys
import os
import re
import argparse
import subprocess
import json
import tempfile
from datetime import datetime, timedelta, timezone


class IpmiFruParser:
    """
    IPMI FRU Information Parser Class
    Loads binary/hex text data, decodes Common Header, Chassis, Board, Product, 
    and MultiRecord areas, and structures the result into a Python dict.
    Includes Celestica DS6000 / DS6001 custom field overlay alignment fixes.
    """
    
    BCD_PLUS = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', '-', '.', '?', '?']
    
    IANA_MAPPING = {
        2: "IBM", 9: "Cisco", 11: "HP / Hewlett-Packard", 343: "Intel Corporation",
        674: "Dell", 807: "Compaq", 1770: "Broadcom", 2384: "Supermicro",
        4123: "NVIDIA", 4314: "Xilinx / AMD", 6745: "American Megatrends Inc. (AMI)",
        10473: "Lenovo", 11102: "Tyan", 11549: "GIGABYTE", 19046: "Mellanox",
        42688: "Hewlett Packard Enterprise", 48924: "Huawei"
    }
    
    CHASSIS_TYPES = {
        0: "Unspecified", 1: "Other", 2: "Unknown", 3: "Desktop", 4: "Low Profile Desktop",
        5: "Pizza Box", 6: "Mini Tower", 7: "Tower", 8: "Portable", 9: "Laptop",
        10: "Notebook", 11: "Hand Held", 12: "Docking Station", 13: "All-in-One",
        14: "Sub Notebook", 15: "Space-saving", 16: "Lunch Box", 17: "Main Server Chassis",
        18: "Expansion Chassis", 19: "SubChassis", 20: "Bus Expansion Chassis",
        21: "Peripheral Chassis", 22: "RAID Chassis", 23: "Rack Mount Chassis",
        24: "Sealed-case PC", 25: "Multi-system Chassis", 26: "Compact PCI",
        27: "Advanced TCA", 28: "Blade", 29: "Blade Enclosure", 30: "Tablet",
        31: "Convertible", 32: "Detachable", 33: "IoT Gateway", 34: "Embedded PC",
        35: "Mini PC", 36: "Stick PC"
    }
    
    MULTIRECORD_TYPES = {
        0: "Power Supply Information", 1: "DC Output", 2: "DC Load",
        3: "Manager Activation Record", 4: "Compatibility Record", 5: "System UUID (OEM)",
        6: "DC Output Capabilities", 7: "DC Load Capabilities", 8: "MAC/Network Info",
        9: "Extended DC Output Info", 10: "Extended DC Load Info", 11: "OEM Network Info",
        12: "Module Current Requirements", 13: "Module Inlet Temperature",
        15: "Channel Info", 16: "Channel Set"
    }

    def __init__(self, disable_overlay=False):
        """
        Initialize the parser.
        :param disable_overlay: Set True to disable DS6000/DS6001 vendor custom overlays.
        """
        self.disable_overlay = disable_overlay
        self.raw_bytes = bytes()
        self.warnings = []
        
        self.result_dict = {
            "common_header": {},
            "chassis_info": {},
            "board_info": {},
            "product_info": {},
            "multirecord": []
        }
        
        self.overlay_active = False
        self.overlay_type = ""
        self.board_labels = []
        self.prod_labels = []

    def _warn(self, msg):
        self.warnings.append(msg)

    @staticmethod
    def _chr_of(c):
        return chr(c) if 32 <= c <= 126 else ' ' if c in (9, 10, 13) else ''

    def _parse_hex_text(self, text):
        """Parse text format like xxd, hexdump -C, or ipmitool hex into raw bytes"""
        hex_digits = []
        for line in text.splitlines():
            line = line.strip()
            if not line:
                continue
            line = re.sub(r'^[0-9A-Fa-f]+([:][\s]*|[\s]{2,})', '', line)
            if '|' in line:
                line = line.split('|')[0]
            line = re.sub(r'\s{3,}.*', '', line)
            
            found = re.findall(r'[0-9A-Fa-f]{2}', line)
            hex_digits.extend(found)
        return bytes(int(x, 16) for x in hex_digits)

    def load_from_file(self, file_path):
        """Load FRU data from file or stdin (if file_path is '-')"""
        if file_path == '-':
            content = sys.stdin.buffer.read()
        else:
            with open(file_path, 'rb') as f:
                content = f.read()
        
        if not content:
            raise ValueError("Failed to read any bytes from input.")

        try:
            text_sample = content.decode('utf-8', errors='strict')
            self.raw_bytes = self._parse_hex_text(text_sample)
        except UnicodeDecodeError:
            self.raw_bytes = content

    def load_from_live_ipmi(self, fru_id=0):
        """Read live system FRU data via external ipmitool command"""
        with tempfile.NamedTemporaryFile(delete=False) as tmp:
            tmp_name = tmp.name
        try:
            cmd = ["ipmitool", "fru", "read", str(fru_id), tmp_name]
            subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            with open(tmp_name, "rb") as f:
                self.raw_bytes = f.read()
        finally:
            if os.path.exists(tmp_name):
                os.unlink(tmp_name)

    def load_from_bytes(self, data_bytes):
        """Directly load raw byte stream data"""
        self.raw_bytes = data_bytes

    def _setup_ds6000_6001_overlay(self):
        """Detect and set up custom label overlays for Celestica DS6000 / DS6001 platforms"""
        if self.disable_overlay:
            return

        sig_chars = [chr(b) if 32 <= b <= 126 else ' ' for b in self.raw_bytes]
        sig = "".join(sig_chars)

        markers = ["DS6000", "DS6001", "R4116-", "R4087-", "R4086-", "R4140-"]
        if not any(m in sig for m in markers):
            return

        self.overlay_active = True
        
        if "DS6000_BMC" in sig or "DS6001_BMC" in sig:
            self.overlay_type = "DS6000/DS6001 BMC"
            self.board_labels = ["PCBA Version", "Last MAC", "UUID", "Reserved"]
        elif any(m in sig for m in ["DS6000_COME", "DS6001_COME", "DS6000_CPU_CARD", "DS6001_CPU_CARD", "ICE-LAKE"]):
            self.overlay_type = "DS6000/DS6001 COM-E"
            self.board_labels = ["Board Name", "PCBA Version"]
        elif any(m in sig for m in ["DS6000_Fan", "DS6001_Fan", "F301D", "F302D"]):
            self.overlay_type = "DS6000/DS6001 Fan Module"
            self.board_labels = ["PCBA Version", "Airflow", "Fan Model"]
        elif any(m in sig for m in ["DS6000_Switch Board", "DS6001_Switch Board", "DS6000_Base Board", "DS6001_Base Board"]):
            self.overlay_type = "DS6000/DS6001 System / Switch Board"
            self.board_labels = ["Board Name", "PCBA Version"]
            self.prod_labels = ["Airflow", "1st MAC", "MAC Size"]
        elif "Fan Control Interface" in sig:
            self.overlay_type = "Fan Control Interface Board"
            self.board_labels = ["PCBA Version"]
        elif "MIDDLE IO" in sig or "Middle IO" in sig:
            self.overlay_type = "Middle IO Board"
            self.board_labels = ["PCBA Version"]
        elif "Timing" in sig:
            self.overlay_type = "Timing Board"
            self.board_labels = ["PCBA Version"]
        elif "LEFT IO" in sig or "Left IO" in sig:
            self.overlay_type = "Left IO Board"
            self.board_labels = ["PCBA Version"]
        elif "Left LED" in sig:
            self.overlay_type = "Left LED Board"
            self.board_labels = ["PCBA Version"]
        elif "Right LED" in sig:
            self.overlay_type = "Right LED Board"
            self.board_labels = ["PCBA Version"]
        elif "SWITCH Board" in sig:
            self.overlay_type = "Switch Board"
            self.board_labels = ["PCBA Version"]
        else:
            self.overlay_type = "DS6000/DS6001 (generic board)"
            self.board_labels = ["PCBA Version"]

    def _get_custom_field_name(self, kind, idx):
        """Map index to mapped custom labels or fall back to default key string"""
        if self.overlay_active:
            labels = self.board_labels if kind == 'board' else self.prod_labels
            if idx < len(labels) and labels[idx] is not None:
                return labels[idx]
        return f"Custom_{idx}"

    def _area_checksum_ok(self, off, length):
        if off + length > len(self.raw_bytes):
            return False
        return (sum(self.raw_bytes[off:off+length]) & 0xFF) == 0

    def _decode_type_length(self, idx):
        if idx >= len(self.raw_bytes):
            return "", idx
        tl = self.raw_bytes[idx]
        length = tl & 0x3F
        ftype = (tl >> 6) & 0x03
        next_idx = idx + 1 + length
        if length == 0:
            return "", next_idx
        if next_idx > len(self.raw_bytes):
            return "", len(self.raw_bytes)

        field_bytes = self.raw_bytes[idx+1:next_idx]
        out = ""
        if ftype == 0:  # BCD plus
            for b in field_bytes:
                out += self.BCD_PLUS[b] if 0 <= b <= 13 else "?"
        elif ftype == 1:  # 6-bit ASCII
            bits, nbits = 0, 0
            for b in field_bytes:
                bits |= (b << nbits)
                nbits += 8
                while nbits >= 6:
                    code = bits & 0x3F
                    bits >>= 6
                    nbits -= 6
                    out += self._chr_of(code + 0x20)
        else:  # 8-bit ASCII / Reserved
            for b in field_bytes:
                out += self._chr_of(b)

        return out.rstrip(), next_idx

    def _parse_chassis(self, off):
        if off + 3 > len(self.raw_bytes): return
        length = self.raw_bytes[off + 1] * 8
        ctype = self.raw_bytes[off + 2]
        pos = off + 3
        
        fields = []
        for _ in range(2):
            val, pos = self._decode_type_length(pos)
            fields.append(val)
        
        customs = []
        while pos < len(self.raw_bytes):
            if self.raw_bytes[pos] == 193:
                break
            val, pos = self._decode_type_length(pos)
            if val: customs.append(val)

        checksum_valid = self._area_checksum_ok(off, length)
        if not checksum_valid:
            self._warn("Chassis Info area checksum failed")

        self.result_dict["chassis_info"] = {
            "type": self.CHASSIS_TYPES.get(ctype, f"Unknown (0x{ctype:02X})"),
            "part_number": fields[0] if len(fields) > 0 else "",
            "serial_number": fields[1] if len(fields) > 1 else "",
            "custom_fields": {f"Custom_{n}": cf for n, cf in enumerate(customs)},
            "checksum_ok": checksum_valid
        }

    def _parse_board(self, off):
        if off + 6 > len(self.raw_bytes): return
        length = self.raw_bytes[off + 1] * 8
        lang = self.raw_bytes[off + 2]
        mins = self.raw_bytes[off + 3] | (self.raw_bytes[off + 4] << 8) | (self.raw_bytes[off + 5] << 16)
        pos = off + 6

        # Standard sequential fields extraction (Strictly follow spec layout sequence)
        fields = []
        for _ in range(5):
            val, pos = self._decode_type_length(pos)
            fields.append(val)

        # Unpacked raw trailing bytes inside block belong to vendor custom area
        customs = []
        while pos < len(self.raw_bytes):
            if self.raw_bytes[pos] == 193:  # 0xC1 EOF
                break
            val, pos = self._decode_type_length(pos)
            if val: customs.append(val)

        # -------------------------------------------------------------------
        # Celestica Fan Module Firmwave Misalignment Post-Fix Patch
        # -------------------------------------------------------------------
        # If the firmware outputted an empty string for standard FRU File ID,
        # but pushed the signature name into the first custom data element position:
        if fields[4] == "" and len(customs) > 0 and any(m in customs[0] for m in ["DS6000", "DS6001"]):
            # Pull the signature name back up into standard fru_file_id position
            fields[4] = customs.pop(0) 
        # -------------------------------------------------------------------

        checksum_valid = self._area_checksum_ok(off, length)
        if not checksum_valid:
            self._warn("Board Info area checksum failed")

        if fields[0] in ["5lestica", "C5lestica"]:
            fields[0] = "Celestica"

        mfg_date_str = None
        if mins > 0:
            base = datetime(1996, 1, 1, 0, 0, 0, tzinfo=timezone.utc)
            mfg_date_str = (base + timedelta(minutes=mins)).strftime('%Y-%m-%d %H:%M UTC')

        board_data = {
            "language": f"0x{lang:02X}",
            "mfg_date": mfg_date_str,
            "manufacturer": fields[0],
            "product_name": fields[1],
            "serial_number": fields[2],
            "part_number": fields[3],       # "R4087-F1004-01"
            "fru_file_id": fields[4],       # "DS6000_Fan"
            "custom_fields": {},
            "checksum_ok": checksum_valid
        }
        
        # Sequentially map remaining content array elements to custom layout mapping structures
        for n, cf in enumerate(customs):
            key_name = self._get_custom_field_name('board', n)
            board_data["custom_fields"][key_name] = cf

        self.result_dict["board_info"] = board_data

    def _parse_product(self, off):
        if off + 3 > len(self.raw_bytes): return
        length = self.raw_bytes[off + 1] * 8
        lang = self.raw_bytes[off + 2]
        pos = off + 3

        fields = []
        for _ in range(7):
            val, pos = self._decode_type_length(pos)
            fields.append(val)

        customs = []
        while pos < len(self.raw_bytes):
            if self.raw_bytes[pos] == 193:
                break
            val, pos = self._decode_type_length(pos)
            if val: customs.append(val)

        checksum_valid = self._area_checksum_ok(off, length)
        if not checksum_valid:
            self._warn("Product Info area checksum failed")

        prod_data = {
            "language": f"0x{lang:02X}",
            "manufacturer": fields[0] if len(fields) > 0 else "",
            "product_name": fields[1] if len(fields) > 1 else "",
            "part_model": fields[2] if len(fields) > 2 else "",
            "version": fields[3] if len(fields) > 3 else "",
            "serial_number": fields[4] if len(fields) > 4 else "",
            "asset_tag": fields[5] if len(fields) > 5 else "",
            "fru_file_id": fields[6] if len(fields) > 6 else "",
            "custom_fields": {},
            "checksum_ok": checksum_valid
        }

        for n, cf in enumerate(customs):
            key_name = self._get_custom_field_name('product', n)
            prod_data["custom_fields"][key_name] = cf

        self.result_dict["product_info"] = prod_data

    def _decode_mr_powersupply(self, s, L):
        sub_dict = {}
        if L < 2: return sub_dict
        le16 = lambda o: self.raw_bytes[o] | (self.raw_bytes[o+1] << 8)
        div10 = lambda v: round(v / 10.0, 1)

        sub_dict["overall_capacity_W"] = div10(le16(s))
        if L >= 4:  sub_dict["peak_va"] = div10(le16(s+2))
        if L >= 6:  sub_dict["inrush_current_A"] = div10(le16(s+4))
        if L >= 7:  sub_dict["inrush_duration_ms"] = self.raw_bytes[s+6]
        if L >= 9:  sub_dict["low_end_voltage_10mV"] = le16(s+7)
        if L >= 11: sub_dict["high_end_voltage_10mV"] = le16(s+9)
        if L >= 12: sub_dict["low_end_freq_dHz"] = self.raw_bytes[s+11]
        if L >= 13: sub_dict["high_end_freq_dHz"] = self.raw_bytes[s+12]
        if L >= 15: sub_dict["dropout_voltage_10mV"] = le16(s+13)
        if L >= 16:
            f = self.raw_bytes[s+15]
            sub_dict["accuracy_pct"] = f & 0x0F
            sub_dict["tolerance_pct"] = (f >> 4) & 0x0F
        if L >= 17:
            pf = self.raw_bytes[s+16]
            sub_dict["temperature_probe_support"] = bool(pf & 0x01)
            sub_dict["voltage_probe_support"] = bool(pf & 0x02)
            sub_dict["fan_probe_support"] = bool(pf & 0x04)
            sub_dict["hot_swap_support"] = bool(pf & 0x08)
            sub_dict["auto_pick_support"] = bool(pf & 0x10)
            sub_dict["predictive_fail_support"] = bool(pf & 0x20)
        if L >= 18: sub_dict["peak_va_holdup_ms"] = self.raw_bytes[s+17]
        if L >= 19:
            v12 = self.raw_bytes[s+18]
            sub_dict["voltage_1_accuracy_10mV"] = v12 & 0x0F
            sub_dict["voltage_2_accuracy_10mV"] = (v12 >> 4) & 0x0F
        if L >= 21: sub_dict["combined_capacity_W"] = div10(le16(s+19))
        if L >= 23: sub_dict["combined_rating_voltage"] = le16(s+21)
        if L >= 25: sub_dict["combined_rating_current"] = le16(s+23)
        return sub_dict

    def _decode_mr_dcoutput(self, s, L):
        sub_dict = {}
        if L < 9: return sub_dict
        le16 = lambda o: self.raw_bytes[o] | (self.raw_bytes[o+1] << 8)
        sub_dict["output_number"] = self.raw_bytes[s]
        sub_dict["standby"] = bool(self.raw_bytes[s+1] & 0x01)
        sub_dict["nominal_voltage_10mV"] = le16(s+2)
        sub_dict["max_current_10mA"] = le16(s+4)
        sub_dict["no_load_current_10mA"] = le16(s+6)
        flags = self.raw_bytes[s+8]
        sub_dict["shared_output"] = bool(flags & 0x01)
        sub_dict["ruggedized"] = bool(flags & 0x02)
        sub_dict["hot_swap"] = bool(flags & 0x04)
        sub_dict["autosupply"] = bool(flags & 0x08)
        return sub_dict

    def _decode_mr_oem(self, s, L):
        sub_dict = {}
        if L < 3:
            sub_dict["raw_data_hex"] = " ".join(f"{b:02X}" for b in self.raw_bytes[s:s+L])
            return sub_dict
        manu = self.raw_bytes[s] | (self.raw_bytes[s+1] << 8) | (self.raw_bytes[s+2] << 16)
        sub_dict["manufacturer_id"] = manu
        sub_dict["manufacturer_name"] = self.IANA_MAPPING.get(manu, "Unknown")
        if L > 3:
            sub_dict["oem_data_hex"] = " ".join(f"{b:02X}" for b in self.raw_bytes[s+3:s+L])
        return sub_dict

    def _parse_multirecord(self, off):
        i, idx, total_len = off, 0, len(self.raw_bytes)
        while i + 5 <= total_len:
            rtype = self.raw_bytes[i]
            rinfo = self.raw_bytes[i+1]
            rlen = self.raw_bytes[i+2]
            rck = self.raw_bytes[i+3]
            eol = (rinfo >> 7) & 0x01
            ver = rinfo & 0x0F
            
            data_start = i + 5
            data_end = data_start + rlen
            
            hdr_ok = (sum(self.raw_bytes[i:i+5]) & 0xFF) == 0
            rec_ok = ((sum(self.raw_bytes[data_start:data_end]) + rck) & 0xFF) == 0
            
            if not rec_ok: self._warn(f"MultiRecord 0x{rtype:02X}: record checksum bad")
            if not hdr_ok: self._warn(f"MultiRecord 0x{rtype:02X}: header checksum bad")

            rec_entry = {
                "record_index": idx,
                "type_code": rtype,
                "type_name": self.MULTIRECORD_TYPES.get(rtype, "OEM/Reserved"),
                "version": ver,
                "is_eol": bool(eol),
                "data_length_bytes": rlen,
                "offset_hex": f"0x{i:X}",
                "checksums_ok": {
                    "header": hdr_ok,
                    "record": rec_ok
                },
                "decoded_fields": {},
                "raw_data_hex": " ".join(f"{b:02X}" for b in self.raw_bytes[data_start:data_end])
            }
                
            if rtype == 0:
                rec_entry["decoded_fields"] = self._decode_mr_powersupply(data_start, rlen)
            elif rtype == 1:
                rec_entry["decoded_fields"] = self._decode_mr_dcoutput(data_start, rlen)
            elif rtype >= 192:
                rec_entry["decoded_fields"] = self._decode_mr_oem(data_start, rlen)
                
            self.result_dict["multirecord"].append(rec_entry)
            
            i = data_end
            idx += 1
            if eol != 0: break
            if idx > 256:
                self._warn("MultiRecord: too many records, aborting")
                break

    def parse(self):
        """
        Main method: Parses FRU bytes and stores into structured self.result_dict
        """
        if len(self.raw_bytes) < 8:
            raise ValueError("FRU data too short to meet Common Header minimum 8-byte requirement.")

        self.warnings = []
        self.result_dict = {"common_header": {}, "chassis_info": {}, "board_info": {}, "product_info": {}, "multirecord": []}
        
        self._setup_ds6000_6001_overlay()

        ver = self.raw_bytes[0]
        off_internal = self.raw_bytes[1] * 8
        off_chassis  = self.raw_bytes[2] * 8
        off_board    = self.raw_bytes[3] * 8
        off_product  = self.raw_bytes[4] * 8
        off_mr       = self.raw_bytes[5] * 8
        pad          = self.raw_bytes[6]
        cksum        = self.raw_bytes[7]
        
        header_total = sum(self.raw_bytes[0:7]) & 0xFF
        expected_cksum = (-header_total) & 0xFF
        header_checksum_ok = (cksum == expected_cksum)
        
        if not header_checksum_ok:
            self._warn("Common Header checksum mismatch")

        self.result_dict["common_header"] = {
            "format_version": ver,
            "platform_eeprom_overlay": self.overlay_type if self.overlay_active else None,
            "offsets": {
                "internal_use_bytes": off_internal,
                "chassis_info_bytes": off_chassis,
                "board_info_bytes": off_board,
                "product_info_bytes": off_product,
                "multirecord_bytes": off_mr
            },
            "header_checksum_ok": header_checksum_ok
        }

        if off_chassis != 0:  self._parse_chassis(off_chassis)
        if off_board != 0:    self._parse_board(off_board)
        if off_product != 0:  self._parse_product(off_product)
        if off_mr != 0:       self._parse_multirecord(off_mr)
        
        self.result_dict["warnings"] = self.warnings

    def get_dict_report(self):
        """Returns the decoded native Python dictionary"""
        return self.result_dict

    def get_json_report(self):
        """Returns serialized JSON string"""
        return json.dumps(self.result_dict, indent=2, ensure_ascii=False)


# ----- CLI Wrapper Main Block -----
def main():
    parser = argparse.ArgumentParser(description="Structured IPMI FRU Parser to Python Dict.")
    parser.add_argument('file', nargs='?', default='-', help="FRU binary, hex dump, or '-' for stdin.")
    parser.add_argument('--ipmi', action='store_true', help="Read live FRU via ipmitool.")
    parser.add_argument('fru_id', type=int, nargs='?', default=0, help="FRU ID for ipmitool.")
    
    args = parser.parse_args()
    fru_parser = IpmiFruParser()
    
    try:
        if args.ipmi:
            fru_parser.load_from_live_ipmi(fru_id=args.fru_id)
        else:
            fru_parser.load_from_file(args.file)
            
        fru_parser.parse()
        print(fru_parser.get_json_report())
            
    except Exception as e:
        sys.stderr.write(f"Error: {e}\n")
        sys.exit(1)


if __name__ == '__main__':
    main()
