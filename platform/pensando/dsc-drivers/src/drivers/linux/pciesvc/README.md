# pciesvc module

## Overview

This driver module is a companion to the kpcimgr driver.  This module
provide support for servicing the pcie bus hardware "indirect" and
"notify" transaction interrupts.  This driver runs on the Pensando ARM cpu.

The core of the driver is built using sources from the pciesvc library
with only a thin wrapper of code here to package the pciesvc core
and register with the kpcimgr driver by calling "kpcimgr_module_register".

## Building

The Makefile in this directory can be used to build the module.
If the kernel build support files are in /lib/modules then "make" will
find them.  If kernel build support files are in another path then
specify on the make command line with "make KDIR=/path/to/kernel".

## History

2022-12-02 - initial version
2023-02-20 - Update to 1.59.0-C-5
2023-05-08 - Update to 1.63.0-C-8
2023-06-23 - Update to 1.65-C-6
2023-09-22 - Update to 1.65-C-35
