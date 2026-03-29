from __future__ import annotations

import os
import time
from sonic_platform import eeprom_utils

BMC_EEPROM_PATH = "/sys/bus/i2c/devices/4-0050/eeprom"

class Eeprom(eeprom_utils.Eeprom):
    """
    BMC SONiC EEPROM handler for the BMC IDPROM on Chipmunk.

    - Uses Nexthop Eeprom subclass so Vendor Extension TLVs
      (IANA 63074 + custom field codes) map to friendly names like
      "Switch Host Serial Number".
    """

    def __init__(self):
        # Nexthop Eeprom ctor matches TlvInfoDecoder(path, start, status, ro)
        if not os.path.exists(BMC_EEPROM_PATH):
            raise RuntimeError(f"EEPROM device not found at {BMC_EEPROM_PATH}")
        super(Eeprom, self).__init__(BMC_EEPROM_PATH, start=0, status="", ro=True)

    def get_eeprom(self):
        """
        Read EEPROM, update Redis, and return raw bytes.
        syseepromd calls this.
        """
        e = self.read_eeprom()
        # Populate STATE_DB: EEPROM_INFO|* keys
        self.update_eeprom_db(e)
        return e

    def read_eeprom(self):
        # Just delegate to base class
        return super(Eeprom, self).read_eeprom()

    def get_system_serial_number(self,
        chip: str = "24c64",
        instantiate_timeout_sec: float = 1.0,
    ) -> str | None:
        """
        Read the system/chassis serial number from the switch card EEPROM.

        Steps:
          - If SWITCH_CARD_EEPROM_PATH exists, read it
          - Otherwise:
            - echo "<chip> 0x50" > /sys/bus/i2c/devices/i2c-10/new_device
            - wait for SWITCH_CARD_EEPROM_PATH to appear
            - read it
            - echo "10-0050" > /sys/bus/i2c/devices/delete_device

        On Nexthop switch cards, TLV 0x23 ("Serial Number") is the *system* serial.

        Returns:
            Serial string if found, else None.
        """
        SWITCH_CARD_EEPROM_I2C_PATH = "/sys/bus/i2c/devices/i2c-10"
        SWITCH_CARD_EEPROM_PATH = "/sys/bus/i2c/devices/10-0050/eeprom"
        created = False

        # Helper: instantiate device if missing
        def ensure_device():
            nonlocal created
            if os.path.exists(SWITCH_CARD_EEPROM_PATH):
                return True

            new_dev_path = SWITCH_CARD_EEPROM_I2C_PATH + "/new_device"
            if not os.path.exists(new_dev_path):
                return False

            try:
                with open(new_dev_path, "w") as f:
                    f.write(f"{chip} 0x50\n")
                created = True
            except OSError:
                return False

            # Poll for eeprom node to appear
            deadline = time.time() + instantiate_timeout_sec
            while time.time() < deadline:
                if os.path.exists(SWITCH_CARD_EEPROM_PATH):
                    return True
                time.sleep(0.05)

            return os.path.exists(SWITCH_CARD_EEPROM_PATH)

        # Helper: best-effort cleanup if we created the device
        def cleanup():
            if not created:
                return
            delete_path_bus = SWITCH_CARD_EEPROM_I2C_PATH + "/delete_device"
            if not os.path.exists(delete_path_bus):
                return
            try:
                with open(delete_path_bus, "w") as f:
                    f.write("10-0050\n")
            except OSError:
                # best effort only
                pass

        if not ensure_device():
            return None

        try:
            with open(SWITCH_CARD_EEPROM_PATH, "rb") as f:
                e = f.read()
        finally:
            cleanup()

        # Parse TlvInfo TLV 0x23
        if len(e) < 11 or e[0:7] != b"TlvInfo":
            return None

        total_len = (e[9] << 8) | e[10]
        idx = 11
        end = 11 + total_len

        while idx + 2 <= len(e) and idx < end:
            t = e[idx]
            l = e[idx + 1]
            vstart = idx + 2
            vend = vstart + l
            if vend > len(e):
                break

            if t == 0x23:  # Serial Number = system/chassis SN
                return e[vstart:vend].decode("ascii", errors="ignore").strip()

            if t == 0xFE:  # CRC TLV
                break

            idx = vend

        return None
