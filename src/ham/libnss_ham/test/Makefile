#*****************************************************************************
#
# AUTHOR: Martin Belanger
#
#*****************************************************************************/
PROGRAM := nsshamtest

PROGRAM_SRC := $(wildcard *.c)

PROGRAM_OBJ := $(patsubst %.c,%.o,$(filter %.c,$(PROGRAM_SRC)))
PROGRAM_DEP := $(PROGRAM_OBJ:.o=.d)

CC      := gcc
#LDFLAGS := $(shell pkg-config --libs libsystemd)
LDFLAGS :=
LL      := gcc
CFLAGS  := -g -O3 -Wall

ifeq (,$(strip $(filter $(MAKECMDGOALS),clean install uninstall package)))
  ifneq (,$(strip $(PROGRAM_DEP)))
    -include $(PROGRAM_DEP)
  endif
endif


# *******************************************************************
# INCLUDES:
# *******************************************************************
# NOTE: D-Bus header files are located in:
# /usr/include/dbus-c++-1/dbus-c++/dbus.h
# /usr/include/dbus-1.0/dbus/dbus.h
#INCLUDES := $(sort $(shell pkg-config --cflags libsystemd))
INCLUDES :=


# *******************************************************************
# Implicit rules:
# *******************************************************************
%.o : %.c
	@printf "%b[1;36m%s%b[0m\n" "\0033" "Compiling: $< -> $@" "\0033"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
	@printf "\n"

%.d : %.c
	@printf "%b[1;36m%s%b[0m\n" "\0033" "Dependency: $< -> $@" "\0033"
	$(CC) -MM -MG -MT '$@ $(@:.d=.o)' $(CFLAGS) $(INCLUDES) -o $@ $<
	@printf "\n"

#####################################################################
#####################################################################

ifeq (,$(strip $(filter $(MAKECMDGOALS),install package)))

# *******************************************************************
# Make all
# *******************************************************************
.DEFAULT_GOAL := all
all: $(PROGRAM)

# *******************************************************************
# PROGRAM
# *******************************************************************
$(PROGRAM): $(PROGRAM_OBJ) $(PROGRAM_DEP) Makefile $(PICS-LIB)
	@printf "%b[1;36m%s%b[0m\n" "\0033" "Linking: $(PROGRAM_OBJ) -> $@" "\0033"
	$(LL) $(CFLAGS) -o $@ $(PROGRAM_OBJ) $(LDFLAGS)
	@printf "%b[1;32m%s%b[0m\n\n" "\0033" "$@ Done!" "\0033"

endif # (,$(strip $(filter $(MAKECMDGOALS),install debian)))

#####################################################################
#####################################################################

# *******************************************************************
# CLEAN
# *******************************************************************
RM_PROGRAM := $(PROGRAM) ./*.o ./*.d

RM_LIST = $(strip $(wildcard $(RM_PROGRAM)))
.PHONY: clean
clean:
	@printf "%b[1;36m%s%b[0m\n" "\0033" "Cleaning" "\0033"
ifneq (,$(RM_LIST))
	rm -rf $(RM_LIST)
endif


