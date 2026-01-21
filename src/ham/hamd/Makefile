# Prerequisite: sudo apt-get install libsystemd-dev libdbus-c++-dev libglib2.0-dev
TARGET    := hamd

SRCS      := $(wildcard *.c) $(wildcard *.cpp)

PKGS      := libsystemd glib-2.0 dbus-c++-glib-1 hiredis
LDLIBS    := $(shell pkg-config --libs   ${PKGS}) -lstdc++fs -lswsscommon
CPPFLAGS  := $(shell pkg-config --cflags ${PKGS}) -DDAEMON_NAME=\"${TARGET}\"
CFLAGS    := -Wall -Werror -O3
CXXFLAGS  := -std=c++14 ${CFLAGS}
LL        := g++

DBUS-GLUE := $(patsubst %.xml,%.dbus-adaptor.h,$(wildcard ../shared/*.xml))

OBJS := \
 $(patsubst %.c,%.o,$(filter %.c,${SRCS})) \
 $(patsubst %.cpp,%.o,$(filter %.cpp,${SRCS}))
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
.DEFAULT_GOAL := all
all: ${TARGET}

# *******************************************************************
# TARGET
${TARGET}: ${OBJS} ${DEPS} Makefile
	@printf "%b[1;36m%s%b[0m\n" "\0033" "Linking: ${OBJS} -> $@" "\0033"
	$(LL) ${LDFLAGS} -o $@ ${OBJS} $(LDLIBS)
	@printf "%b[1;32m%s%b[0m\n\n" "\0033" "$@ Done!" "\0033"

endif # (,$(strip $(filter $(MAKECMDGOALS),install debian)))


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

%.o : %.c
	@printf "%b[1;36m%s%b[0m\n" "\0033" "Compiling: $< -> $@" "\0033"
	${CC} ${CPPFLAGS} ${CFLAGS} -c $< -o $@
	@printf "\n"

%.d : %.c
	@printf "%b[1;36m%s%b[0m\n" "\0033" "Dependency: $< -> $@" "\0033"
	${CC} -MM -MG -MT '$@ $(@:.d=.o)' ${CPPFLAGS} ${CFLAGS} -o $@ $<
	@printf "\n"

# Implicit rule to generate DBus header files from XML
../shared/%.dbus-adaptor.h: ../shared/%.xml
	@printf "%b[1;36m%s%b[0m\n" "\0033" "dbusxx-xml2cpp $< --adaptor=$@" "\0033"
	@dbusxx-xml2cpp $< --adaptor=$@
    # The following sed commands are used to add gcc warning suppression for:
    #     warning: variable 'ri' set but not used [-Wunused-but-set-variable]
	sed -i '1s/^/#pragma GCC diagnostic push\n#pragma GCC diagnostic ignored "-Wunused-but-set-variable"/' $@
	sed -i '$$s/$$/\n#pragma GCC diagnostic pop/' $@

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


