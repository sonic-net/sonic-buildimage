# SONiC build glue for obmc_ipmi BMC RTC updater.
#

CC ?= gcc
CFLAGS ?= -Wall -Werror -O2

BIN := obmc_rtc_updater

all: $(BIN)

$(BIN): obmc_rtc_updater.c libobmc_ipmi.c libobmc_ipmi.h
	$(CC) $(CFLAGS) -o $@ obmc_rtc_updater.c libobmc_ipmi.c

clean:
	$(RM) $(BIN)

.PHONY: all clean
