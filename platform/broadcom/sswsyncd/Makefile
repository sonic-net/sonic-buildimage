CC = g++
CFLAGS_COMMON = -ansi -fPIC -std=c++11 -Wall -Wcast-align -Wcast-qual \
    -Wconversion -Wdisabled-optimization -Werror -Wextra -Wfloat-equal \
    -Wformat=2 -Wformat-nonliteral -Wformat-security -Wformat-y2k \
    -Wimport -Winit-self -Winline -Winvalid-pch -Wlong-long \
    -Wmissing-field-initializers -Wmissing-format-attribute \
    -Wmissing-include-dirs -Wmissing-noreturn -Wno-aggregate-return \
    -Wno-padded -Wno-switch-enum -Wno-unused-parameter -Wpacked \
    -Wpointer-arith -Wredundant-decls -Wshadow -Wstack-protector \
    -Wstrict-aliasing=3 -Wswitch -Wswitch-default -Wunreachable-code \
    -Wunused -Wvariadic-macros -Wwrite-strings -Wno-switch-default \
    -Wconversion -Wlong-long

CFLAGS += -g -std=c++11
LIBS += -lpthread -lutil

DSSERVE = dsserve
BCMCMD = bcmcmd

.PHONY: all clean install

all: $(DSSERVE) $(BCMCMD)

$(DSSERVE): dsserve.cpp dsserve.h
	$(CC) -o $@ $< $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(LIBS)

$(BCMCMD): bcmcmd.cpp dsserve.h
	$(CC) -o $@ $< $(CFLAGS) $(CFLAGS_COMMON) $(CPPFLAGS) $(LDFLAGS) $(LIBS)

install:
	install -d $(DESTDIR)/usr/bin
	install -D $(DSSERVE) $(DESTDIR)/usr/bin/$(DSSERVE)
	install -D $(BCMCMD) $(DESTDIR)/usr/bin/$(BCMCMD)

clean:
	rm -f $(DSSERVE) $(BCMCMD)
