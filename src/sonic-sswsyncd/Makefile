CFLAGS_COMMON =
CFLAGS_COMMON += -ansi
CFLAGS_COMMON += -fPIC
CFLAGS_COMMON += -std=c++11
CFLAGS_COMMON += -Wall
CFLAGS_COMMON += -Wcast-align
CFLAGS_COMMON += -Wcast-qual
CFLAGS_COMMON += -Wconversion
CFLAGS_COMMON += -Wdisabled-optimization
CFLAGS_COMMON += -Werror
CFLAGS_COMMON += -Wextra
CFLAGS_COMMON += -Wfloat-equal
CFLAGS_COMMON += -Wformat=2
CFLAGS_COMMON += -Wformat-nonliteral
CFLAGS_COMMON += -Wformat-security
CFLAGS_COMMON += -Wformat-y2k
CFLAGS_COMMON += -Wimport
CFLAGS_COMMON += -Winit-self
CFLAGS_COMMON += -Winline
CFLAGS_COMMON += -Winvalid-pch
CFLAGS_COMMON += -Wlong-long
CFLAGS_COMMON += -Wmissing-field-initializers
CFLAGS_COMMON += -Wmissing-format-attribute
CFLAGS_COMMON += -Wmissing-include-dirs
CFLAGS_COMMON += -Wmissing-noreturn
CFLAGS_COMMON += -Wno-aggregate-return
CFLAGS_COMMON += -Wno-padded
CFLAGS_COMMON += -Wno-switch-enum
CFLAGS_COMMON += -Wno-unused-parameter
CFLAGS_COMMON += -Wpacked
CFLAGS_COMMON += -Wpointer-arith
CFLAGS_COMMON += -Wredundant-decls
CFLAGS_COMMON += -Wshadow
CFLAGS_COMMON += -Wstack-protector
CFLAGS_COMMON += -Wstrict-aliasing=3
CFLAGS_COMMON += -Wswitch
CFLAGS_COMMON += -Wswitch-default
CFLAGS_COMMON += -Wunreachable-code
CFLAGS_COMMON += -Wunused
CFLAGS_COMMON += -Wvariadic-macros
CFLAGS_COMMON += -Wwrite-strings
CFLAGS_COMMON += -Wno-switch-default
CFLAGS_COMMON += -Wconversion
CFLAGS_COMMON += -Wlong-long

CC = g++
SAI_INCLUDE_DIR = /usr/include/sai
CFLAGS = -g -I$(SAI_INCLUDE_DIR) -I/usr/include/libnl3 -std=c++11
ifdef SAITHRIFT
SAI_RPC_INCLUDE_DIR = libsaiserver/include
SAI_RPC_LIB_DIR = libsaiserver/lib
CFLAGS += -I$(SAI_RPC_INCLUDE_DIR) -L$(SAI_RPC_LIB_DIR) -DSAITHRIFT=yes
endif
ifeq ($(platform),MLNX)
CDEFS = -DMLNXSAI
else
ifeq ($(platform),BFT)
CDEFS = -DBFTSAI
else
ifeq ($(platform),)
CDEFS = -DBRCMSAI
endif
endif
endif
INSTALL := /usr/bin/install
DSSERVE := dsserve
BCMCMD := bcmcmd

SDIR = src
ODIR = obj
BIN_DIR = build

LIBS = -lpthread -lutil

$(shell mkdir -p $(ODIR) && mkdir -p $(BIN_DIR))

.phony : install clean

all : $(DSSERVE) $(BCMCMD)

$(DSSERVE) : $(SDIR)/$(DSSERVE).cpp
	$(CC) -o $(BIN_DIR)/$(DSSERVE) $^ $(CFLAGS) $(LIBS)

$(BCMCMD) : $(SDIR)/$(BCMCMD).cpp
	$(CC) -o $(BIN_DIR)/$(BCMCMD) $^ $(CFLAGS) $(CFLAGS_COMMON) $(LIBS)

install :
	$(INSTALL) -D $(BIN_DIR)/$(DSSERVE) $(DESTDIR)/usr/sbin/$(DSSERVE)
	$(INSTALL) -D $(BIN_DIR)/$(BCMCMD) $(DESTDIR)/usr/sbin/$(BCMCMD)

clean:
	rm -rf $(ODIR) $(BIN_DIR)
