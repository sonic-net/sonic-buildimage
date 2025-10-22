bsp_base-objs = bsp_base_sysfs.o bspbase.o
syseeprom-objs = syseeprom_sysfs.o syseeprom_base.o
psu-objs = psu_sysfs.o psu_base.o
cpld-objs = cpld_sysfs.o cpld_base.o

obj-m:=bsp_base.o syseeprom.o cpld.o drv_cpld.o
SHELL := /bin/bash
KVER :=$(shell uname -r)
#KVER :=4.9.0-9-2-amd64
KDIR :=/lib/modules/$(KVER)/build
PWD := $(shell pwd)
#EXTRA_CFLAGS += -D_PDO
EXTRA_CFLAGS += -Wframe-larger-than=4096

