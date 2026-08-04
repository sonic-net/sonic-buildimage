Juniper Networks Platform Support for SONiC
===========================================

This readme provides information on how to install and upgrade ONIE and SONiC images on the Juniper Networks switches. 

Note: Switches ship with ONIE and SONiC images preinstalled. 

## Supported platforms

The following Juniper Networks platforms are supported for the SONiC operating system:

 - QFX5210-64C-S
 - QFX5200-32C-S


## Building and Installing ONIE

ONIE is the bootloader used and it's a prerequisite to install ONIE on the switches before installing SONiC.

1. Cross compile ONIE

To compile ONIE, you need to change the directories to "build-config" and then based on the platform issue the following commands

a) For QFX5210-64C-S platform, invoke "make MACHINEROOT=../machine/juniper MACHINE=juniper_qfx5210 all".

For example:

```
  $ cd build-config
  $ make -j4 MACHINEROOT=../machine/juniper MACHINE=juniper_qfx5210 all
```

b) For QFX5200-32C-S platform, invoke "make MACHINEROOT=../machine/juniper MACHINE=juniper_qfx5200 all".

ONIE binaries are located at the directory /build/images. The following command shows how to navigate the directory to view the ONIE binaries:

```
stack@controller:~/ONIE_J/onie/build/images$ ls -rlt

total 40740

-rw-rw-r-- 1 stack stack  3710240 Aug  3 12:32 juniper_x86-r0.vmlinuz   -- ONIE kernel image
-rw-rw-r-- 1 stack stack  6038416 Aug  3 12:32 juniper_x86-r0.initrd    -- ONIE initramfs (filesystem)
-rw-rw-r-- 1 stack stack  9811831 Aug  3 12:32 onie-updater-x86_64-juniper_x86-r0    -- ONIE self-update image for installing ONIE.
-rw-rw-r-- 1 stack stack 22151168 Aug  3 12:33 onie-recovery-x86_64-juniper_x86-r0.iso   -- Recovery ISO image to create a bootable USB memory device for installing/recovery ONIE.
-rw-rw-r-- 1 stack stack 31465984 Aug  3 12:33 onie-recovery-x86_64-juniper_qfx5210-r0.efi64.pxe   -- Recovery PXE image used for installing ONIE using PXE Network install.
```


Note: Use the following command to build a demo target:

For example:

```
  $ make -j4 MACHINEROOT=../machine/juniper MACHINE=juniper_qfx5210 all demo
```

In addition to the above list of binary files, the following two binary files are also created:

```
-rw-rw---- 1 build build 12576008 Aug 19 16:30 demo-installer-x86_64-juniper_qfx5210-r0.bin
-rw-rw---- 1 build build 12576008 Aug 19 16:30 demo-diag-installer-x86_64-juniper_qfx5210-r0.bin
```

You can install these binary files by using the 'onie-nos-install' command to test the install / boot workflow.

Use the following command for make clean:

For example:

```
 $ make machine-clean MACHINEROOT=../machine/juniper MACHINE=juniper_qfx5210
```


## Installing ONIE

To install ONIE on a new switch, you can use one of the following ONIE recovery images:

1) .<machine>.iso       -- Hybrid ISO image.
2) .<machine>.efi64.pxe -- PXE image for UEFI64 machines.

Note: Second method is not applicable for QFX5200-32C-S


## Creating an ISO Recovery Image

You can use the recovery ISO (.iso) image to create a bootable USB memory device.

To create a bootable USB memory device, use the "dd" command on a Linux workstation as follows:

```
 $ dd if=<machine>.iso of=/dev/sdX bs=10M
```

For example:

```
 $ dd if=onie-recovery-x86_64-juniper_qfx5210-r0.iso of=/dev/sdb bs=10M
```

You can find the correct "/dev/sdX" by validating the "dmesg" output after inserting an USB device into the Linux workstation.


1) Booting from a USB Memory Device

To boot from an external USB memory device connected to the switch, you need to:

a. Insert the USB memory device to the USB port of the switch.

b. Power on the switch and enter the BIOS configuration by pressing the Esc key, as displayed in the console screen.

c. Set the hard drive boot order as follows:

   When you see the "Boot Option #1" displayed on the console screen, select the USB memory device:

    Boot-->Boot Option Priorities-->Boot Option #1

   If the USB memory device name is not listed in "Boot Option #1", check the priorities in the hard drive boot order:

    Boot-->Hard Drive BBS Priorities-->Boot Option #1


   For example, consider "JetFlashTranscend 8GB 8.07" as the USB memory device, the boot order will display as follows:

```
    Boot Option Priorities

    Boot Option #1          [JetFlashTranscend 8...]

    Boot Option #2          [ATP ATP IG eUSB 1100]

    Boot Option #3          [IBA GE Slot 00A0 v1543]

    Boot Option #4          [UEFI: Built-in EFI ...]
```

d. Go to "Save & Exit" in the BIOS screen and from the Boot Override option select the USB memory device (For example, JetFlashTranscend 8GB 8.07).

e. After a few seconds, the switch would restart and boot from the USB memory device and then you will see the following on the console screen:

```
                     GNU GRUB  version 2.02~beta2+e4a1fe391

 +----------------------------------------------------------------------------+
 |*ONIE: Rescue                                                               |
 | ONIE: Embed ONIE                                                           |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 +----------------------------------------------------------------------------+

      Use the ^ and v keys to select which entry is highlighted.
      Press enter to boot the selected OS, `e' to edit the commands before booting or `c' for a command-line.
```

f. Select "ONIE: Embed ONIE" to create a fresh partition to install ONIE automatically. 
	
	Warning: All the data on the hard disk drive will be erased.

g. Select "ONIE: Rescue" to enter the ONIE recovery command-line shell (Optional).



2) Recovering ONIE using PXE-UEFI64 Recovery Image

Note: This section is only applicable for QFX5210-64C-S platform.

You can use the onie-recovery-x86_64-juniper_qfx5210-r0.efi64.pxe image to recover the ONIE image through UEFI PXE.

   The onie-recovery-x86_64-juniper_qfx5210-r0.efi64.pxe is made for the QFX5210-64C-S switch that has a PXE client which is based on UEFI64.
   The onie-recovery-x86_64-juniper_qfx5210-r0.efi64.pxe is a combination of grub-efi-64 and the .iso recovery image, that looks like an UEFI application. The UEFI PXE client on the QFX5210-64C-S can then boot it.

   For more information on UEFI PXE Netboot, see https://wiki.ubuntu.com/UEFI/PXE-netboot-install.

   Note: To install the PXE-UEFI64 recovery image over the network, you need to configure your DHCP server so that DHCP clients receives the onie-recovery-x86_64-juniper_qfx5210-r0.efi64.pxe image as the bootfile.

   To enable IPv4 PXE boot on the QFX5210-64C-S switch:

	1) Enter the BIOS configuration.
	2) Click "Save & Exit" menu on the "Boot Override" option.
	3) Select "UEFI: IP4 Broadcom NetXtreme Gigabit Ethernet"

## Related Documentation for ONIE:

The following links provide more information about ONIE:

	1. ONIE documentation: https://opencomputeproject.github.io/onie/. 
	2. How to build and install ONIE on QFX5210-64C-S switch, see 		https://github.com/opencomputeproject/onie/blob/master/machine/juniper/juniper_qfx5210/INSTALL. 

## SONiC Build Process:

The instruction on how to build an ONIE compatible network operating system (NOS) installer image for Juniper Networks switches, and how to build docker images running inside the NOS is available at https://github.com/Azure/sonic-buildimage#usage. 


## Install SONiC on the Juniper Networks switch:es

You need to copy the SONiC image 'sonic-broadcom.bin' to the switch. You can copy the sonic-broadcom.bin to an USB memory device and insert it to the USB port of the switch. You can also use the 'scp' command to copy the sonic-broadcom.bin image to the switch over the network.

Note: Unmount the USB memory device after copying the sonic-broadcom.bin. For example, umount /dev/sdX, where X is the name of the drive of the USB memory device.

Run the following command to install SONIC:
   
```
For example, 
ONIE:/var/tmp # onie-nos-install /var/tmp/sonic-broadcom.bin
```

## Booting SONiC 

The switch restarts automatically after the SONiC image has been successfully installed. 

1) Select SONiC from the GRUB boot manager. 

```
                      GNU GRUB  version 2.02

 +----------------------------------------------------------------------------+
 |*SONiC-OS-master.0-dirty-20190913.060138                                    | 
 | ONIE                                                                       |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 |                                                                            | 
 +----------------------------------------------------------------------------+

      Use the ^ and v keys to select which entry is highlighted.          
      Press enter to boot the selected OS, `e' to edit the commands       
      before booting or `c' for a command-line. 
```

2. At the SONiC login prompt, enter the username as admin and password as YourPaSsWoRd.

You can now start configuring the Juniper Networks switch running SONiC as its operating system. 


## Upgrading SONiC image

To upgrade the SONiC operating system to a latest version, you need to: 

 1. Copy the latest image of the SONiC image to the switch. 
 2. Run the following command from the directory where the latest SONiC image has been copied.

```
$ sudo ./sonic-braodcom.bin 
```

or

```
$ sudo sonic-installer ./sonic-broadcom.bin -y
```


## Uninstalling SONiC image

To unintall SONiC operating system from the switch, you need to:

 1. Reboot the switch.
 2. Go to the ONIE GRUB menu and then select ONIE: Uninstall OS option to uninstall SONiC.

For more details on drivers and platform scripts see the following links:

1) QFX5210-64C-S: https://github.com/Azure/sonic-buildimage/blob/master/platform/broadcom/sonic-platform-modules-juniper/qfx5210/utils/README

2) QFX5200-32C-S: https://github.com/Azure/sonic-buildimage/blob/master/platform/broadcom/sonic-platform-modules-juniper/qfx5200/utils/README

## Related Documentation for SONiC:

The following links provide more information about SONiC:
 1. SONiC documentation: https://github.com/azure/sonic/wiki. 

## Viewing the Device Revision of the FRU Model from IDEEPROM

You can view the device revisions of the FRU model from IDEEPROM by using the show platform syseeprom CLI command.

Note: The Device version shown is the HEX ASCII equivalent of the FRU model. For example, if the device version shows 41, then the HEX ASCII equivalent is character A. 
