#############################################################################
# SONiC Virtual switch platform
#
#############################################################################

class board():

    def __init__(self, name, path, cpld_root, ro):
        return

    def check_status(self):
        return "ok"

    def is_checksum_valid(self, e):
        return (True, 0)

    def read_eeprom(self):
        return \
            """
TLV Name             Code Len Value
-------------------- ---- --- -----
Product Name         0x21   5 SONiC
Part Number          0x22   6 000000
Serial Number        0x23  20 0000000000000000000
Base MAC Address     0x24   6 00:00:00:00:00:01
Manufacture Date     0x25  19 10/19/2019 00:00:03
Device Version       0x26   1 1
Label Revision       0x27   3 A01
Platform Name        0x28  20 x86_64-kvm_x86_64-r0
ONIE Version         0x29  19 master-201811170418
MAC Addresses        0x2A   2 16
Vendor Name          0x2D   5 SONiC
"""

    def decode_eeprom(self, e):
        print(e)

    def serial_number_str(self, e):
        """Return service tag instead of serial number"""
        return "000000"
