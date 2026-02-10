# Prerequisite: sudo apt-get install libsystemd-dev libdbus-c++-dev
TARGET    := libham.so
SRCS      := $(wildcard *.cpp)

#PKGS      := libsystemd dbus-c++-glib-1
PKGS      := dbus-c++-glib-1
LDFLAGS   := -shared -Wl,-soname,${TARGET}
LDLIBS    := $(shell pkg-config --libs   ${PKGS})
CPPFLAGS  := $(shell pkg-config --cflags ${PKGS})
CFLAGS    := -fPIC -Wall -Werror -O3
CXXFLAGS  := -std=c++11 ${CFLAGS}
LL        := g++

DBUS-GLUE := $(patsubst %.xml,%.dbus-proxy.h,$(wildcard ../shared/*.xml))

OBJS := $(patsubst %.cpp,%.o,$(filter %.cpp,${SRCS}))
DEPS := $(OBJS:.o=.d)

ifeq (,$(strip $(filter $(MAKECMDGOALS),clean install uninstall package)))
  ifneq (,$(strip ${DEPS}))
    ${DEPS}: ${DBUS-GLUE}
    -include ${DEPS}
  endif
endif


ifeq (,$(strip $(filter $(MAKECMDGOALS),clean install uninstall)))
# *******************************************************************
# Make all
# *******************************************************************
.DEFAULT_GOAL := all
all: ${TARGET}

${TARGET}: ${OBJS} Makefile
	@printf "%b[1;36m%s%b[0m\n" "\0033" "Building $@" "\0033"
	$(LL) ${LDFLAGS} -o $@ ${OBJS} $(LDLIBS)
	@printf "%b[1;32m%s%b[0m\n\n" "\0033" "$@ Done!" "\0033"

endif # (,$(strip $(filter $(MAKECMDGOALS),clean install uninstall)))


# *******************************************************************
# Implicit rules:
# *******************************************************************
%.o : %.cpp
	@printf "%b[1;36m%s%b[0m\n" "\0033" "Compiling: $< -> $@" "\0033"
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -c $< -o $@
	@printf "\n"

%.d : %.cpp
	@printf "%b[1;36m%s%b[0m\n" "\0033" "Dependency: $< -> $@" "\0033"
	${CXX} -MM -MG -MT '$@ $(@:.d=.o)' ${CPPFLAGS} ${CXXFLAGS} -o $@ $<
	@printf "\n"

# Implicit rule to generate DBus header files from XML
../shared/%.dbus-proxy.h: ../shared/%.xml
	@printf "%b[1;36m%s%b[0m\n" "\0033" "dbusxx-xml2cpp $< --proxy=$@" "\0033"
	@dbusxx-xml2cpp $< --proxy=$@

# *******************************************************************
#   ____ _
#  / ___| | ___  __ _ _ __
# | |   | |/ _ \/ _` | '_ \
# | |___| |  __/ (_| | | | |
#  \____|_|\___|\__,_|_| |_|
#
# *******************************************************************
RM_TARGET := ${TARGET} ./*.o ./*.d ${DBUS-GLUE}

RM_LIST = $(strip $(wildcard ${RM_TARGET}))
.PHONY: clean
clean:
ifneq (,$(RM_LIST))
	rm -rf $(RM_LIST)
endif

