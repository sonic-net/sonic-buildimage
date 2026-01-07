RM := rm -rf
EVENTD_TARGET := eventd
EVENTD_TEST := tests/tests
EVENTD_TOOL := tools/events_tool
EVENTD_PUBLISH_TOOL := tools/events_publish_tool.py
RSYSLOG-PLUGIN_TARGET := rsyslog_plugin/rsyslog_plugin
RSYSLOG-PLUGIN_TEST := rsyslog_plugin_tests/tests
EVENTD_MONIT := tools/events_monit_test.py
EVENTD_MONIT_CONF := tools/monit_events

CP := cp
MKDIR := mkdir
CC := g++
LIBS := -levent -lhiredis -lswsscommon -lpthread -lboost_thread -lboost_system -lzmq -lboost_serialization -luuid -llua5.1
TEST_LIBS := -L/usr/src/gtest -lgtest -lgtest_main -lgmock -lgmock_main

CFLAGS += -Wall -std=c++17 -fPIE -I$(PWD)/../sonic-swss-common/common
PWD := $(shell pwd)

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS) $(OBJS)
endif
endif

-include src/subdir.mk
-include tests/subdir.mk
-include tools/subdir.mk
-include rsyslog_plugin/subdir.mk
-include rsyslog_plugin_tests/subdir.mk

all: sonic-eventd eventd-tool rsyslog-plugin

test: eventd-tests rsyslog-plugin-tests

sonic-eventd: $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: G++ Linker'
	$(CC) $(LDFLAGS) -o $(EVENTD_TARGET) $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

eventd-tool: $(TOOL_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: G++ Linker'
	$(CC) $(LDFLAGS) -o $(EVENTD_TOOL) $(TOOL_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

rsyslog-plugin: $(RSYSLOG-PLUGIN_OBJS)
	@echo 'Buidling Target: $@'
	@echo 'Invoking: G++ Linker'
	$(CC) $(LDFLAGS) -o $(RSYSLOG-PLUGIN_TARGET) $(RSYSLOG-PLUGIN_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

eventd-tests: $(TEST_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: G++ Linker'
	$(CC) $(LDFLAGS) -o $(EVENTD_TEST) $(TEST_OBJS) $(LIBS) $(TEST_LIBS)
	@echo 'Finished building target: $@'
	$(EVENTD_TEST)
	@echo 'Finished running tests'
	@echo ' '

rsyslog-plugin-tests: $(RSYSLOG-PLUGIN-TEST_OBJS)
	@echo 'BUILDING target: $@'
	@echo 'Invoking G++ Linker'
	$(CC) $(LDFLAGS) -o $(RSYSLOG-PLUGIN_TEST) $(RSYSLOG-PLUGIN-TEST_OBJS) $(LIBS) $(TEST_LIBS)
	@echo 'Finished building target: $@'
	$(RSYSLOG-PLUGIN_TEST)
	@echo 'Finished running tests'
	@echo ' '

install:
	$(MKDIR) -p $(DESTDIR)/usr/bin
	$(MKDIR) -p $(DESTDIR)/etc/monit/conf.d
	$(CP) $(EVENTD_TARGET) $(DESTDIR)/usr/bin
	$(CP) $(EVENTD_TOOL) $(DESTDIR)/usr/bin
	$(CP) $(EVENTD_PUBLISH_TOOL) $(DESTDIR)/usr/bin
	$(CP) $(RSYSLOG-PLUGIN_TARGET) $(DESTDIR)/usr/bin
	$(CP) $(EVENTD_MONIT) $(DESTDIR)/usr/bin
	$(CP) $(EVENTD_MONIT_CONF) $(DESTDIR)/etc/monit/conf.d

deinstall:
	$(RM) -rf $(DESTDIR)/usr
	$(RM) -rf $(DESTDIR)/etc

clean:
	-$(RM) $(EVENTD_TARGET) $(OBJS) $(EVENTD_TOOL) $(TOOL_OBJS) $(RSYSLOG-PLUGIN_TARGET) $(RSYSLOG-PLUGIN_OBJS) $(EVENTD_TEST) $(TEST_OBJS) $(RSYSLOG-PLUGIN_TEST) $(RSYSLOG-PLUGIN-TEST_OBJS)
	-@echo ' '

.PHONY: all clean dependents
