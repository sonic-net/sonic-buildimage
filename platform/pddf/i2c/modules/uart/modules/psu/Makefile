# obj-m := uart_psu_module.o

# ccflags-y := -I$(M)/modules/include

# all:
# 	make -C /lib/modules/5.10.0-8-2-amd64/build M=$(PWD) modules

# clean:
# 	make -C /lib/modules/5.10.0-8-2-amd64/build M=$(PWD) clean

obj-m := driver/
obj-m += uart_psu_module.o

ccflags-y:= -I$(M)/modules/include

