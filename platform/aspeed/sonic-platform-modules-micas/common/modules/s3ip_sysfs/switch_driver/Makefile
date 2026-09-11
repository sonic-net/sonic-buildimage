PWD		= $(shell pwd)

MAKEFILE_FILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_HEADER_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../include)
SYSFS_DRIVER_HEADER_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../sysfs_driver/include)
GCU_DRIVER_HEADER_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../gcu/include)
DPU_DRIVER_HEADER_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../dpu/include)
PUBLIC_HEADER_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../../../public/include)

EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -I$(CURRENT_HEADER_DIR)
EXTRA_CFLAGS+= -I$(SYSFS_DRIVER_HEADER_DIR)
EXTRA_CFLAGS+= -I$(GCU_DRIVER_HEADER_DIR)
EXTRA_CFLAGS+= -I$(DPU_DRIVER_HEADER_DIR)
EXTRA_CFLAGS+= -I$(PUBLIC_HEADER_DIR)
EXTRA_CFLAGS+= -Wall

SUBDIR_CFG = cfg
SUBDIR_GCU = gcu
SUBDIR_NVME = nvme
SUBDIR_DPU = dpu

obj-m := wb_switch_driver.o
wb_switch_driver-objs := switch_driver.o wb_module.o \
wb_fan_driver.o \
wb_eeprom_driver.o \
wb_cpld_driver.o \
wb_fpga_driver.o \
wb_led_driver.o \
wb_slot_driver.o \
wb_sensors_driver.o \
wb_psu_driver.o \
wb_sff_driver.o \
wb_watchdog_driver.o \
wb_system_driver.o \
wb_cabletray_driver.o\
wb_misc_fw_driver.o\
wb_leak_detector_driver.o \
wb_sol_driver.o \
wb_debug_driver.o \
wb_lsw_driver.o \
wb_pcie_driver.o \
wb_clock_driver.o \
wb_pll_driver.o \
wb_avs_driver.o \
wb_disk_driver.o \
wb_md_driver.o \
$(SUBDIR_CFG)/dfd_cfg.o \
$(SUBDIR_CFG)/dfd_cfg_adapter.o \
$(SUBDIR_CFG)/dfd_cfg_file.o \
$(SUBDIR_CFG)/dfd_cfg_info.o \
$(SUBDIR_CFG)/dfd_cfg_listnode.o \
$(SUBDIR_CFG)/dfd_frueeprom.o \
$(SUBDIR_CFG)/dfd_wbtlv_eeprom.o \
$(SUBDIR_CFG)/dfd_tlveeprom.o \
$(SUBDIR_GCU)/wb_gcu_common.o \
$(SUBDIR_GCU)/wb_gcu_l600_l900_driver.o \
$(SUBDIR_NVME)/wb_nvme_driver.o \
$(SUBDIR_DPU)/wb_dpu_common.o \
$(SUBDIR_DPU)/wb_dpu_jaguar_driver.o \

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules_install
clean:
	rm -f $(PWD)/*.o $(PWD)/$(SUBDIR_CFG)/*.o $(PWD)/*.ko $(PWD)/*.mod.c $(PWD)/.*.cmd $(PWD)/$(SUBDIR_CFG)/.*.cmd
	rm -f $(PWD)/Module.markers $(PWD)/Module.symvers $(PWD)/modules.order
	rm -rf $(PWD)/.tmp_versions
