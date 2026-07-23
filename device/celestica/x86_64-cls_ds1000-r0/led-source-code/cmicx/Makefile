#
# Copyright: (c) 2012 Broadcom Corp.
# All Rights Reserved.
#

SDK = ../../../../sdk-all-6.5.16/
export SDK

ifeq (${SDK},)
# {
$(error environment variable SDK not set)
# }
endif

INCDIR	= -I${SDK}/include

all:
	arm-linux-gcc -O1 -g ${INCDIR} -mthumb -mcpu=cortex-m0 -c -o custom_led.o custom_led.c
	arm-linux-ld -T custom_led.lds -Bstatic -nostdlib --entry 0 -EL -Map custom_led.map -o custom_led.elf custom_led.o
	arm-linux-objcopy -O binary custom_led.elf custom_led.bin

clean:
	rm -rf *.elf *.o *.map *.bin
