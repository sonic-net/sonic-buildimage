# $Id$
# $Copyright: (c) 2020 Broadcom.
# Broadcom Proprietary and Confidential. All rights reserved.$
#
# Default rule to build dependencies.  This builds an x.d file for each
# x.c file that describes the dependencies.  We then conditionally include 
# the generated .d files.
#

ifneq (n/a,$(PHYMOD_DEPEND))

#
# If making 'clean', do not include any .d files.  If they are included,
# gmake intrinsically tries to remake them all.
#

ifeq (,$(findstring clean,$(MAKECMDGOALS)))

#
# Attempt to build the depend files.  If it fails, the depend file is
# removed so that it is not included in later builds.
#
$(BLDDIR)/%.d : %.c $(BLDDIR)/.tree
	@$(ECHO) Dependencies for $(LOCALDIR)/$<
ifdef MAKEDEP
	@$(MAKEDEP) "$@" "$@" "$(PHYMOD_DEPEND)"
else
	@$(PRINTF) '$$(BLDDIR)/' > $@
	@$(PHYMOD_DEPEND) >> $@
endif

ifneq ($(strip $(LSRCS)),)
ifneq (,$(findstring .$(OBJSUFFIX),$(MAKECMDGOALS)))
-include $(addprefix $(BLDDIR)/,$(MAKECMDGOALS:.$(OBJSUFFIX)=.d))
else
-include $(addprefix $(BLDDIR)/,$(addsuffix .d,$(basename $(LSRCS))))
endif
endif

endif	# !CLEANING

clean::
	@$(ECHO) Cleaning dependencies for $(LOCALDIR)
	@$(RM) $(BOBJS:.$(OBJSUFFIX)=.d)

endif
