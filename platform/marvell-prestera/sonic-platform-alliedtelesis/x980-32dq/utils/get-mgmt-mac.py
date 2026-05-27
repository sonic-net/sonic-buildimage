#!/usr/bin/env python3


def main():
    try:
        import subprocess
        import sonic_platform
        from sonic_eeprom import eeprom_base
    except ImportError as e:
        raise ImportError(str(e) + "- required module not found")

    platform = sonic_platform.platform.Platform().get_chassis()
    base_mac = platform._eeprom.base_mac_addr()
    mgmt_mac = increment_mac(base_mac)

    subprocess.run(["/sbin/ip", "link", "set", "eth0", "address", mgmt_mac], check=True)

    return 0

def increment_mac(mac):
    if mac != "":
        mac_octets = []
        mac_octets = mac.split(':')
        ret_mac = int(mac_octets[5], 16) | int(mac_octets[4], 16) << 8 | int(mac_octets[3], 16) << 16
        ret_mac = ret_mac + 1
        if (ret_mac & 0xff000000):
            print('Error: increment carries into OUI')
            return ''
        mac_octets[5] = hex(ret_mac & 0xff)[2:].zfill(2)
        mac_octets[4] = hex((ret_mac >> 8) & 0xff)[2:].zfill(2)
        mac_octets[3] = hex((ret_mac >> 16) & 0xff)[2:].zfill(2)
        return ':'.join(mac_octets).upper()
    return ''

if __name__ == "__main__":
    exit(main())
