STAMP = .dirstamp

.DEFAULT_GOAL = main_target

ifeq ($(MAIN_TARGET), credo)
SRCDIR = $(SRC_ROOT)/sdk
else
#Building HAL
CPPFLAGS += -fvisibility=hidden -D BUILD_HAL
SRCDIR = $(SRC_ROOT)/chips/$(MAIN_TARGET)  $(SRC_ROOT)/libs
endif

ifdef CREDO_SDK_REV
CPPFLAGS += -D CREDO_SDK_REV="\"${CREDO_SDK_REV}\"" -D  CREDO_SDK_REV_MAJOR=$(CREDO_SDK_REV_MAJOR) -D  CREDO_SDK_REV_MINOR=$(CREDO_SDK_REV_MINOR) -D  CREDO_SDK_REV_PATCH=$(CREDO_SDK_REV_PATCH)
endif

SRCDIR += $(addprefix $(SRC_ROOT)/ip/,$(IP))
VPATH = $(SRCDIR) $(PARTIAL_IP_DIRS)
OBJDIR = $(OBJ_ROOT)/$(MAIN_TARGET)
IMGDIR = $(IMG_ROOT)
#CPPFLAGS += -fvisibility=hidden -D PROJECT=$(MAIN_TARGET)
CPPFLAGS += -D PROJECT=$(MAIN_TARGET)

### Rules from file list

COBJS = $(foreach dir,$(SRCDIR),$(patsubst %.c,%.o,$(wildcard $(dir)/*.c))) $(PARTIAL_IP_OBJS)

OBJS = $(foreach obj,$(COBJS), $(addprefix $(OBJDIR)/,$(notdir $(obj))))

### list of makefiles

MAKEFILE := $(MAKEFILE_LIST) $(ROOT_DIR)/Makefile

### Setup

ifneq ($(MAKECMDGOALS), clean)
-include $(OBJS:%.o=%.dep)
endif

.SECONDEXPANSION:

### Main target for workaround
main_target: $(IMGDIR)/$(MAIN_TARGET).$(SHLIBEXT)

### Basic rules

# Making directory
%/$(STAMP):
	@mkdir -p $(@D)
	@touch $@

# C source file
$(OBJDIR)/%.o: %.c $(MAKEFILE) | $$(@D)/$(STAMP)
	@echo "Compile $(shell basename $<)"
	$(Q)$(CC) -MMD -MF $(OBJDIR)/$*.dep -MT $@ $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.dep: %.c $(MAKEFILE) | $$(@D)/$(STAMP)
	$(Q)$(CC) -MM -MG -MF $@ -MT $(OBJDIR)/$*.o $(CPPFLAGS) $(CFLAGS) $<

include/%_version.h.gen: include/release_version.h.tmpl FORCE
	@../utils/gen_version.pl --verbose --template $< --target $* > $@.tmp || exit 1; \
	if diff -q $@.tmp $@ > /dev/null 2>&1; then \
		rm $@.tmp ;\
	else \
		echo Generating $@; \
		mv $@.tmp $@ ;\
	fi

# Various target nicknames
# Metas
# real targets

$(TARGETS:%=%-release):	%-release:
	@if git diff --quiet && git diff --quiet --cached; \
	then \
		echo Releasing...; \
		$(MAKE) $*-release-force; \
	else \
		echo Current working directory not clean. Stop.; \
		exit 1; \
	fi

$(TARGETS:%=%-objlist): %-objlist: FORCE
	@echo $* = $($*)

listobj:
	@echo Objects for $(MAIN_TARGET): $(OBJS)

# Normal elf target
# DLLFLAGS specified to link to credo dll when building
$(IMGDIR)/%.$(SHLIBEXT): $(OBJS) | $(IMGDIR)/$(STAMP)
	@echo "Build lib$(@F)"
ifndef WASM_BUILD
	$(Q)$(CC) -shared $(LDFLAGS) $(OBJS) $(DLLFLAGS) -o $(@D)/lib$(@F) $(LDLIBS)
else
	$(Q)$(CC) -s SIDE_MODULE=1 -s WASM_BIGINT $(LDFLAGS) $(OBJS) $(DLLFLAGS) -o $(@D)/lib$(@F) $(LDLIBS)
endif
ifndef NO_STATIC_LIB
ifeq ($(SHLIBEXT),so)
	@echo "Build lib$(basename $(@F)).o"
	$(Q)$(LD) -r $(OBJS) -o $(OBJDIR)/lib$(basename $(@F)).o
ifneq ($(MAIN_TARGET), credo) # on hal layer hide symbols
	$(Q)$(OBJCOPY) --localize-hidden $(OBJDIR)/lib$(basename $(@F)).o
	$(Q)$(OBJCOPY) -w --globalize-symbol "__x86.*" $(OBJDIR)/lib$(basename $(@F)).o
endif
	@echo "Build lib$(basename $(@F)).a"
	$(Q)$(AR) rcs $(@D)/lib$(basename $(@F)).a $(OBJDIR)/lib$(basename $(@F)).o
endif
endif

#######################################

clean:
	$(RM) $(OBJDIR)/*.o $(IMGDIR)/*.so $(IMGDIR)/*.dll $(IMGDIR)/*.a

cleandep:
	$(RM) $(OBJDIR)/*.{o,dep} $(IMGDIR)/*.so $(IMGDIR)/*.a

.PHONY: FORCE
