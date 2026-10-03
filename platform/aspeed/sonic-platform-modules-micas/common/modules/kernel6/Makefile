# Kernel 6.x specific drivers.
# Example:
# obj-m += wb_example_k6.o
obj-m += wb_gpio_pca953x.o

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install