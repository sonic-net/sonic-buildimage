# TARGET = uart_psu_driver_module

# obj-m := $(TARGET).o

# $(TARGET)-objs := uart_psu_api.o uart_psu_driver.o

# ccflags-y := -I$(KBUILD_EXTMOD)/../include

# all:
# 	make -C /lib/modules/5.10.0-8-2-amd64/build M=$(PWD) modules

# clean:
# 	make -C /lib/modules/5.10.0-8-2-amd64/build M=$(PWD) clean

TARGET = uart_psu_driver_module
obj-m := $(TARGET).o 

$(TARGET)-objs := uart_psu_api.o uart_psu_driver.o 

ccflags-y := -I$(M)/modules/include
