# $Id$
# $Copyright: (c) 2020 Broadcom.
# Broadcom Proprietary and Confidential. All rights reserved.$
#
# PHYMOD default make rules. These can optionally be overridden by
# letting PHYMOD_MAKE_RULES point to a different rules file.
#

ifdef PHYMOD_MAKE_RULES

include $(PHYMOD_MAKE_RULES)

else

$(BLDDIR)/%.$(OBJSUFFIX): %.c $(BLDDIR)/.tree
	@$(ECHO) 'Compiling $(LOCALDIR)/$<'
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) -c -g $< -o $@

$(LIBDIR)/$(LIBNAME).$(LIBSUFFIX): $(BOBJS) $(BLDDIR)/.tree 
	@$(ECHO) 'Building library $(LIBNAME)...'
	$(Q)$(AR) $(ARFLAGS) $@ $(BOBJS)

endif
