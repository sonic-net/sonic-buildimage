ifeq ($(CONFIGURED_ARCH), arm64)
EXTRA_CFLAGS += -DSOC_ACTIVE
EXTRA_CFLAGS += -DDMA_MEM_MODE_PLATFORM
endif

obj-m := dal.o
dal-y := dal_kernel.o dal_mpool.o
