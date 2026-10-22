#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
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
"""Scenario groups: name -> (PLATFORM, steps).  Each group runs in its own
process per implementation, from a fresh setup.sh."""
import json, os, sys

F = '/tmp/c/fw/'
def new(i, k, idx=0): return ['new', i, k, idx]
def call(i, m, *a): return ['call', i, m, list(a)]
import re as _re
def sh(c): return ['shell', _re.sub(r"(?<![\w$.])/(tmp|mnt|lib/onie|var/run|usr/bin/asic_detect|usr/share|etc/sonic)", r"$R/\1", c)]
def ini(name, pairs): return ['ini', '/tmp/c/ini/' + name, pairs]

std = lambda i: [call(i, 'get_name'), call(i, 'get_description')]
def onie_read(i):
    return [sh("touch /tmp/c/mount_copy"), call(i, 'get_firmware_version'), sh("rm -rf /mnt/onie-fs; rm -f /tmp/c/mount_copy")]

G = {}

# configparser corpus, then the ONIE/SSD/BIOS/CPLD components on a plain switch.
ini_files = {
    'a.ini': '[version]\nCPLD1 = v1\ncpld2: v2\n  cont\n\n  cont2\n[DEFAULT]\nd=dv\n[firmware]\nburn = %(d)s.vme\nx = 100%%\ny = %(missing)s\nz = %(burn)s/%(d)s\nw = %%\nv = %(\n',
    'b.ini': '\n# c\n[s]\na=1\n[s]\n',
    'c.ini': 'x=1\n',
    'd.ini': '[s]\nk1 = a\nnovalue\n\tk2\n=v\n[t]\nk = 1\nK = 2\n',
    'e.ini': '[s] trailing\na:=b\n[[weird]]\nc = d ; not a comment\n',
    'f.ini': '[s]\na = %(b)s\nb = %(a)s\n',
    'g.ini': '[s]\r\na = 1\r\n',
    'h.ini': '',
}
setup_ini = 'mkdir -p /tmp/c/ini; ' + '; '.join("printf %s > /tmp/c/ini/%s" % (json.dumps(v).replace('%', '%%').replace('$', '\\$'), k) for k, v in ini_files.items())
pairs = [['version', 'CPLD1'], ['version', 'cpld2'], ['firmware', 'burn'], ['firmware', 'x'], ['firmware', 'y'], ['firmware', 'z'],
         ['firmware', 'w'], ['firmware', 'v'], ['DEFAULT', 'd'], ['', 'd'], ['nosec', 'a'], ['s', 'a'], ['s', 'k1'], ['s', 'k2'],
         ['s', ''], ['t', 'k'], ['s', 'b'], ['[weird]', 'c'], ['weird', 'c'], ['s', 'a:']]
G['generic'] = ('x86_64-nvidia_sn5600-r0', [sh(setup_ini)] + [ini(k, pairs) for k in sorted(ini_files)] + [ini('missing.ini', pairs)] + [
    ['host_list'],
    # ONIE
    new('o', 'ONIE'), *std('o'), *onie_read('o'),
    call('o', 'get_available_firmware_version', F + 'onie-updater-x86_64-mlnx_x86-r0'),
    call('o', 'get_firmware_update_notification', F + 'x'),
    sh("printf 'ver=1\\n' > /tmp/c/fw/onie2; printf 'image_version=a=b\\n' > /tmp/c/fw/onie3; printf 'echo image_version=9; exit 4' > /tmp/c/fw/onie4"),
    call('o', 'get_available_firmware_version', F + 'onie2'), call('o', 'get_available_firmware_version', F + 'onie3'),
    call('o', 'get_available_firmware_version', F + 'onie4'), call('o', 'get_available_firmware_version', F + 'nonexistent'),
    sh("sed -i 's/ONIE boot/ONIE xboot/' /tmp/c/fdisk"), call('o', 'get_firmware_version'),
    sh("sed -i 's/ONIE xboot/ONIE boot/' /tmp/c/fdisk; echo 3 > /tmp/c/rc_fdisk"), call('o', 'get_firmware_version'),
    sh("rm /tmp/c/rc_fdisk; echo 32 > /tmp/c/rc_mount"), call('o', 'get_firmware_version'),
    sh("rm /tmp/c/rc_mount; mkdir -p /mnt/onie-fs; ln -s /nowhere /lib/onie"), call('o', 'get_firmware_version'),
    sh("mkdir -p /mnt/onie-fs; ln -s /nowhere /lib/onie"), *onie_read('o'),
    sh("cp /tmp/c/onie-fs/onie/grub/grub-machine.cfg /tmp/c/gm; printf 'onie_version=a=b\\n' > /tmp/c/onie-fs/onie/grub/grub-machine.cfg"), *onie_read('o'),
    sh("printf 'onie_machine=x\\n' > /tmp/c/onie-fs/onie/grub/grub-machine.cfg"), *onie_read('o'),
    sh("rm /tmp/c/onie-fs/onie/grub/grub-machine.cfg"), *onie_read('o'), call('o', 'get_firmware_version'),
    sh("cp /tmp/c/gm /tmp/c/onie-fs/onie/grub/grub-machine.cfg"),
    call('o', 'install_firmware', F + 'missing'),
    call('o', 'install_firmware', F + 'onie-updater-x86_64-mlnx_x86-r0'),
    sh("printf 'Name | Version | Date\\n====+====\\nold.bin | 1 | d\\n00-x.rom | 2 | d\\n** footer\\n' > /tmp/c/pending"),
    call('o', 'install_firmware', F + 'onie-updater-x86_64-mlnx_x86-r0'),
    sh("echo 1 > /tmp/c/rc_remove"), call('o', 'install_firmware', F + 'onie-updater-x86_64-mlnx_x86-r0'),
    sh("rm /tmp/c/rc_remove; echo 'No pending firmware updates present' > /tmp/c/pending; echo 2 > /tmp/c/rc_add"),
    call('o', 'install_firmware', F + 'onie-updater-x86_64-mlnx_x86-r0'),
    sh("rm /tmp/c/rc_add; touch /tmp/c/stage_adds; echo 5 > /tmp/c/rc_update"), call('o', 'update_firmware', F + 'onie-updater-x86_64-mlnx_x86-r0'),
    sh("echo 'No pending firmware updates present' > /tmp/c/pending; echo kill > /tmp/c/rc_update"), call('o', 'install_firmware', F + 'onie-updater-x86_64-mlnx_x86-r0'),
    sh("echo 6 > /tmp/c/rc_show"), call('o', 'install_firmware', F + 'onie-updater-x86_64-mlnx_x86-r0'),
    sh("rm -f /tmp/c/rc_show /tmp/c/rc_update /tmp/c/stage_adds; echo 'No pending firmware updates present' > /tmp/c/pending"),
    call('o', 'auto_update_firmware', F + 'missing', 'cold'), call('o', 'auto_update_firmware', F + 'onie-updater-x86_64-mlnx_x86-r0', 'warm'),
    call('o', 'auto_update_firmware', F + 'onie-updater-x86_64-mlnx_x86-r0', 'cold'),
    sh("echo 2 > /tmp/c/rc_add"), call('o', 'auto_update_firmware', F + 'onie-updater-x86_64-mlnx_x86-r0', 'cold'), sh("rm /tmp/c/rc_add"),
    # SSD
    new('s', 'SSD'), *std('s'), call('s', 'get_firmware_version'),
    call('s', 'get_available_firmware_version', F + 'ssd.pkg'), call('s', 'get_firmware_update_notification', F + 'ssd.pkg'),
    call('s', 'auto_update_firmware', F + 'ssd.pkg', 'warm'), call('s', 'auto_update_firmware', F + 'ssd.pkg', 'cold'),
    sh("printf 'Firmware Version: 0115\\nUpgrade Required: no\\n' > /tmp/c/ssd_qi"),
    call('s', 'get_available_firmware_version', F + 'ssd.pkg'), call('s', 'get_firmware_update_notification', F + 'ssd.pkg'),
    call('s', 'auto_update_firmware', F + 'ssd.pkg', 'warm'),
    sh("printf 'Available Firmware Version:   0202\\nUpgrade Required: yes\\nPower Cycle Required: no\\n' > /tmp/c/ssd_qi"),
    call('s', 'get_available_firmware_version', F + 'ssd.pkg'), call('s', 'get_firmware_update_notification', F + 'ssd.pkg'),
    call('s', 'auto_update_firmware', F + 'ssd.pkg', 'cold'),
    sh("printf 'Upgrade Required: yes\\nPower Cycle Required: maybe\\n' > /tmp/c/ssd_qi"),
    call('s', 'get_available_firmware_version', F + 'ssd.pkg'), call('s', 'get_firmware_update_notification', F + 'ssd.pkg'),
    call('s', 'auto_update_firmware', F + 'ssd.pkg', 'cold'),
    sh("printf 'Upgrade Required: perhaps\\n' > /tmp/c/ssd_qi"), call('s', 'get_available_firmware_version', F + 'ssd.pkg'),
    sh("printf 'Firmware Version: 1\\nUpgrade Required: no\\nPower Cycle Required\\n' > /tmp/c/ssd_qi"), call('s', 'get_firmware_update_notification', F + 'ssd.pkg'),
    sh("printf 'Upgrade Required\\n' > /tmp/c/ssd_qi"), call('s', 'get_firmware_update_notification', F + 'ssd.pkg'),
    call('s', 'auto_update_firmware', F + 'ssd.pkg', 'cold'),
    sh("echo 7 > /tmp/c/rc_ssd_qi"), call('s', 'get_available_firmware_version', F + 'ssd.pkg'), call('s', 'auto_update_firmware', F + 'ssd.pkg', 'cold'),
    sh("rm /tmp/c/rc_ssd_qi; printf 'Model: x\\n' > /tmp/c/ssd_q"), call('s', 'get_firmware_version'),
    sh("printf 'Firmware Version\\n' > /tmp/c/ssd_q"), call('s', 'get_firmware_version'),
    sh("echo 1 > /tmp/c/rc_ssd_q"), call('s', 'get_firmware_version'),
    call('s', 'install_firmware', F + 'ssd.bin'), call('s', 'install_firmware', F + 'ssd.pkg'),
    sh("echo 9 > /tmp/c/rc_ssd_install"), call('s', 'install_firmware', F + 'ssd.pkg'), call('s', 'update_firmware', F + 'ssd.pkg'),
    sh("rm /tmp/c/rc_ssd_install"), call('s', 'update_firmware', F + 'ssd.pkg'),
    # BIOS
    new('b', 'BIOS'), *std('b'), call('b', 'get_firmware_version'), call('b', 'get_available_firmware_version', F + 'bios.rom'),
    call('b', 'get_firmware_update_notification', F + 'bios.rom'),
    call('b', 'install_firmware', F + 'bios.rom'), call('b', 'install_firmware', F + 'bios.cab'), call('b', 'install_firmware', F + 'bios.bin'),
    call('b', 'update_firmware', F + 'bios.rom'), call('b', 'auto_update_firmware', F + 'bios.rom', 'cold'),
    sh("echo 2 > /tmp/c/rc_add"), call('b', 'install_firmware', F + 'bios.rom'), sh("rm /tmp/c/rc_add"),
    sh("sed -i 's/5.3.0021/5.1.0016/' /tmp/c/onie-fs/onie/grub/grub-machine.cfg"), call('b', 'install_firmware', F + 'bios.rom'),
    sh("printf 'onie_version=garbage\\n' > /tmp/c/onie-fs/onie/grub/grub-machine.cfg"), call('b', 'install_firmware', F + 'bios.rom'),
    sh("cp /tmp/c/gm /tmp/c/onie-fs/onie/grub/grub-machine.cfg; echo 3 > /tmp/c/rc_dmi"), call('b', 'get_firmware_version'),
    sh("rm /tmp/c/rc_dmi"),
    # CPLD, not SPC1
    new('c1', 'CPLD', 1), new('c2', 'CPLD', 2), new('c3', 'CPLD', 3), new('c4', 'CPLD', 4), *std('c1'),
    call('c1', 'get_firmware_version'), call('c2', 'get_firmware_version'), call('c3', 'get_firmware_version'), call('c4', 'get_firmware_version'),
    call('c1', 'get_firmware_update_notification', F + 'cpld.vme'), call('c1', 'get_firmware_update_notification', F + 'good.mpfa'),
    call('c1', 'get_available_firmware_version', F + 'good.mpfa'), call('c2', 'get_available_firmware_version', F + 'good.mpfa'),
    call('c3', 'get_available_firmware_version', F + 'good.mpfa'), call('c1', 'get_available_firmware_version', F + 'good.tgz'),
    call('c1', 'get_available_firmware_version', F + 'none.mpfa'), call('c1', 'get_available_firmware_version', F + 'nometa.mpfa'),
    call('c1', 'get_available_firmware_version', F + 'broken.mpfa'), call('c1', 'get_available_firmware_version', F + 'nohdr.mpfa'),
    call('c1', 'get_available_firmware_version', F + 'dup.mpfa'), call('c1', 'get_available_firmware_version', F + 'dupsec.mpfa'),
    call('c1', 'get_available_firmware_version', F + 'parse.mpfa'), call('c1', 'get_available_firmware_version', F + 'interp.mpfa'),
    call('c2', 'get_available_firmware_version', F + 'interp.mpfa'), call('c3', 'get_available_firmware_version', F + 'interp.mpfa'),
    call('c4', 'get_available_firmware_version', F + 'interp.mpfa'), call('c1', 'get_available_firmware_version', F + 'multi.mpfa'),
    call('c2', 'get_available_firmware_version', F + 'multi.mpfa'),
    call('c1', 'install_firmware', F + 'cpld.vme'), call('c1', 'install_firmware', F + 'cpld.bin'), call('c1', 'install_firmware', F + 'missing.vme'),
    call('c1', 'install_firmware', F + 'good.mpfa'), call('c1', 'install_firmware', F + 'noburn.mpfa'), call('c1', 'install_firmware', F + 'parse.mpfa'),
    call('c1', 'install_firmware', F + 'multi.mpfa'), call('c1', 'install_firmware', F + 'interp.mpfa'),
    sh("echo 1 > /tmp/c/rc_cpld"), call('c1', 'install_firmware', F + 'cpld.vme'), call('c1', 'auto_update_firmware', F + 'cpld.vme', 'cold'),
    call('c1', 'update_firmware', F + 'good.mpfa'), sh("rm /tmp/c/rc_cpld"),
    call('c1', 'auto_update_firmware', F + 'cpld.vme', 'cold'), call('c1', 'auto_update_firmware', F + 'cpld.vme', 'fast'),
    call('c1', 'auto_update_firmware', F + 'nope.vme', 'cold'),
    call('c1', 'update_firmware', F + 'good.mpfa'), call('c1', 'update_firmware', F + 'norefresh.mpfa'), call('c1', 'update_firmware', F + 'noburn.mpfa'),
    call('c1', 'update_firmware', F + 'cpld.vme'), call('c1', 'update_firmware', F + 'none.mpfa'),
    # CPLD on SPC1: cpldupdate through mst
    sh("echo spc1 > /tmp/c/asic"), call('c1', 'install_firmware', F + 'cpld.vme'),
    sh("echo 1 > /tmp/c/rc_cpld"), call('c1', 'install_firmware', F + 'cpld.vme'), sh("rm /tmp/c/rc_cpld"),
    sh("echo 2 > /tmp/c/rc_mst_start"), call('c1', 'install_firmware', F + 'cpld.vme'), sh("rm /tmp/c/rc_mst_start"),
    sh("echo 2 > /tmp/c/rc_mst_stop"), call('c1', 'install_firmware', F + 'cpld.vme'), sh("rm /tmp/c/rc_mst_stop"),
    sh("echo 3 > /tmp/c/rc_asic_m"), call('c1', 'install_firmware', F + 'cpld.vme'), sh("rm /tmp/c/rc_asic_m"),
    sh(": > /tmp/c/mstdev"), call('c1', 'install_firmware', F + 'cpld.vme'),
    sh("printf '\\377\\n' > /tmp/c/mstdev"), call('c1', 'install_firmware', F + 'cpld.vme'),
    sh("echo /dev/mst/mt53100_pci_cr0 > /tmp/c/mstdev; echo 1 > /tmp/c/rc_asic; printf ' spc1 \\n' > /tmp/c/asic"), call('c1', 'install_firmware', F + 'cpld.vme'),
    sh("rm /tmp/c/rc_asic; echo spc4 > /tmp/c/asic; chmod -x /usr/bin/asic_detect/asic_detect.sh"), call('c1', 'install_firmware', F + 'cpld.vme'),
    sh("rm /usr/bin/asic_detect/asic_detect.sh"), call('c1', 'install_firmware', F + 'cpld.vme'),
    sh("cp /tmp/c/orig/asic_detect.sh /usr/bin/asic_detect/asic_detect.sh; echo spc1 > /tmp/c/asic"),
    call('c1', 'install_firmware', F + 'cpld.vme'),
    # what the chassis builds when the CPLD count is unreadable
    sh("echo x > /var/run/hw-management/config/cpld_num"), ['host_list'],
    sh("rm /var/run/hw-management/config/cpld_num"), ['host_list'],
    sh(": > /var/run/hw-management/config/cpld_num"), ['host_list'],
    sh("echo 12 > /var/run/hw-management/config/cpld_num"), ['host_list'],
    # /lib/onie a directory: the symlink fails, naming both paths
    sh("mkdir -p /lib/onie"), call('o', 'get_firmware_version'),
    # the staged copy's name taken by a directory
    sh("rmdir /lib/onie; rm -f /tmp/00-onie-updater-x86_64-mlnx_x86-r0; mkdir -p /tmp/00-onie-updater-x86_64-mlnx_x86-r0"),
    call('o', 'install_firmware', F + 'onie-updater-x86_64-mlnx_x86-r0'),
    sh("rmdir /tmp/00-onie-updater-x86_64-mlnx_x86-r0; mkfifo /tmp/00-onie-updater-x86_64-mlnx_x86-r0"),
    call('o', 'install_firmware', F + 'onie-updater-x86_64-mlnx_x86-r0'),
    sh("rm /tmp/00-onie-updater-x86_64-mlnx_x86-r0"),
])

G['sn2201'] = ('x86_64-nvidia_sn2201-r0', [
    ['host_list'],
    new('b', 'BIOSSN2201'), *std('b'), call('b', 'get_firmware_version'), call('b', 'get_available_firmware_version', F + 'bios.rom'),
    call('b', 'get_firmware_update_notification', F + 'bios.rom'), call('b', 'install_firmware', F + 'bios.rom'),
    call('b', 'update_firmware', F + 'bios.rom'), call('b', 'auto_update_firmware', F + 'bios.rom', 'cold'),
    call('b', 'auto_update_firmware', F + 'bios.rom', 'warm'), call('b', 'auto_update_firmware', F + 'nope', 'cold'),
    sh("printf 'BIOS Information\\n\\tVendor: AMI\\n' > /tmp/c/dmi_t0"), call('b', 'get_firmware_version'),
    sh("echo 1 > /tmp/c/rc_dmi"), call('b', 'get_firmware_version'), sh("rm /tmp/c/rc_dmi"),
    new('B', 'BIOS'), call('B', 'install_firmware', F + 'bios.rom'),
    new('c', 'CPLDSN2201', 1), *std('c'), call('c', 'get_firmware_version'),
    call('c', 'install_firmware', F + 'cpld.bin'), call('c', 'install_firmware', F + 'missing.vme'), call('c', 'install_firmware', F + 'good.mpfa'),
    call('c', 'update_firmware', F + 'good.mpfa'), sh("echo 1 > /tmp/c/rc_cpld"), call('c', 'update_firmware', F + 'good.mpfa'),
    call('c', 'auto_update_firmware', F + 'cpld.vme', 'cold'), sh("rm /tmp/c/rc_cpld"),
])

G['sn4280'] = ('x86_64-nvidia_sn4280-r0', [
    ['host_list'],
    new('c', 'CPLDSN4280', 2), call('c', 'install_firmware', F + 'cpld.bin'), call('c', 'update_firmware', F + 'good.mpfa'),
    new('f', 'FPGADPU', 1), new('f3', 'FPGADPU', 3), *std('f'), call('f', 'get_firmware_version'), call('f3', 'get_firmware_version'),
    call('f', 'get_available_firmware_version', F + 'good.mpfa'), call('f3', 'get_available_firmware_version', F + 'good.mpfa'),
    call('f', 'get_firmware_update_notification', F + 'good.mpfa'),
    call('f', 'install_firmware', F + 'cpld.bin'),
    sh("echo 4 > /tmp/c/rc_mst_start"), call('f', 'install_firmware', F + 'cpld.bin'), sh("rm /tmp/c/rc_mst_start"),
    sh("echo 1 > /tmp/c/rc_cpld"), call('f', 'install_firmware', F + 'cpld.bin'), call('f', 'update_firmware', F + 'good.mpfa'), sh("rm /tmp/c/rc_cpld"),
    # dpu0 and dpu3 running before the burn; dpu0 drops during it and is back 7s
    # later, so the post-update hook polls until it is.  (The wait running out
    # -- 300s, then "did not reach OS_RUN; rebooting anyway" -- is too long to
    # run here.)
    sh("printf '%s\\n' 'echo 3 > $R/var/run/hw-management/dpu1/system/boot_progress' "
       "'(sleep 7; echo 5 > $R/var/run/hw-management/dpu1/system/boot_progress) > /dev/null 2>&1 &' > /tmp/c/cpld_hook"),
    call('f', 'update_firmware', F + 'good.mpfa'),
    sh("rm /tmp/c/cpld_hook"),
    sh("echo 2 > /tmp/c/rc_reboot"), call('f', 'update_firmware', F + 'good.mpfa'),
    sh("rm /tmp/c/rc_reboot; echo x > /var/run/hw-management/dpu4/system/boot_progress"), call('f', 'update_firmware', F + 'norefresh.mpfa'),
    # an MPFA without a burn entry
    call('f', 'update_firmware', F + 'noburn.mpfa'),
    sh("echo x > /var/run/hw-management/config/dpu_num"), ['host_list'],
])

G['simx'] = ('x86_64-nvidia_sn4800_simx-r0', [['host_list']])

G['bmc'] = ('x86_64-nvidia_sn6600_ld-r0', [
    sh("mkdir -p /etc/sonic; cp /usr/share/sonic/platform/bmc.json /etc/sonic/bmc.json"),
    new('c', 'CPLD', 1), call('c', 'update_firmware', F + 'good.mpfa'), sh("cat /var/run/hw-management/system/aux_pwr_cycle; echo"),
    sh("rm -f /var/run/hw-management/system/aux_pwr_cycle; mkdir /var/run/hw-management/system/aux_pwr_cycle"), call('c', 'update_firmware', F + 'good.mpfa'),
    ['host_list'],
])

if __name__ == '__main__':
    if sys.argv[1:] == ['names']:
        print(' '.join(G))
    else:
        name = sys.argv[1]
        print(G[name][0])
        json.dump(G[name][1], open(sys.argv[2], 'w'), indent=0)
