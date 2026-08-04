CC := g++

RSYSLOG-PLUGIN-TEST_OBJS += ./rsyslog_plugin/rsyslog_plugin.o ./rsyslog_plugin/syslog_parser.o ./rsyslog_plugin/timestamp_formatter.o
RSYSLOG-PLUGIN_OBJS += ./rsyslog_plugin/rsyslog_plugin.o ./rsyslog_plugin/syslog_parser.o ./rsyslog_plugin/timestamp_formatter.o ./rsyslog_plugin/main.o

C_DEPS += ./rsyslog_plugin/rsyslog_plugin.d ./rsyslog_plugin/syslog_parser.d ./rsyslog_plugin/timestamp_formatter.d ./rsyslog_plugin/main.d

rsyslog_plugin/%.o: rsyslog_plugin/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(CC) -D__FILENAME__="$(subst rsyslog_plugin/,,$<)" $(CFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$(@)" "$<"
	@echo 'Finished building: $<'
	@echo ' '
