CC=gcc
RDIR=.
EPDM_HOME =
VSRCS += $(wildcard $(RDIR)/src/*.c)
LSRCS = $(notdir $(VSRCS))
LOCALDIR = $(RDIR)/src
BLDDIR = $(RDIR)/src/obj
LIBDIR = $(RDIR)/lib
LOBJS = $(addsuffix .o, $(basename $(LSRCS)))
BOBJS = $(addprefix $(BLDDIR)/,$(LOBJS))
LIBNAME = libsmbus
LIBSUFFIX = a
CFLAGS = -g -Wall -Werror -Wno-uninitialized -fPIC

#ADD_FLAGS += -DPLP_APERTA2_SUPPORT
#ADD_FLAGS += -DPLP_AGERA2_SUPPORT

$(shell mkdir -p $(BLDDIR))
$(shell mkdir -p $(LIBDIR))

INCLUDE = -I $(RDIR)/inc -I $(EPDM_HOME)
ECHO = echo

$(BLDDIR)/%.o: $(LOCALDIR)/%.c
	@$(ECHO) 'Compiling $(LOCALDIR)/$<'
	$(CC) $< $(INCLUDE) $(CFLAGS) $(ADD_FLAGS) -c -o $@

$(LIBDIR)/$(LIBNAME).$(LIBSUFFIX): $(BOBJS)
	@$(ECHO) 'Building library $(LIBNAME)...'
	$(Q)$(AR) $(ARFLAGS) $@ $(BOBJS)

BLIBNAME = $(LIBDIR)/$(LIBNAME).$(LIBSUFFIX)

all:: $(BLIBNAME)

clean::
	@$(ECHO) Cleaning objects for $(notdir $(BLIBNAME))
	$(RM) $(BOBJS) $(BLIBNAME)
	$(RM) -rf $(BLDDIR) $(LIBDIR)
