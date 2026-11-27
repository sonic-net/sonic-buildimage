RM := rm -rf
PCIMEM_TARGET := pcimem
CP := cp
MKDIR := mkdir
CC := gcc
MV := mv

-include src/subdir.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

all: pcimem-build 

pcimem-build: $(OBJS)
	$(CC) -o "$(PCIMEM_TARGET)" $(OBJS)

install:
	$(MKDIR) -p $(DESTDIR)/usr/sbin
	$(MV) $(PCIMEM_TARGET) $(DESTDIR)/usr/sbin

deinstall:
	$(RM) $(DESTDIR)/usr/sbin/$(PCIMEM_TARGET)
	$(RM) -rf $(DESTDIR)/usr/sbin

clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) $(PCIMEM_TARGET)

.PHONY: all clean dependents
