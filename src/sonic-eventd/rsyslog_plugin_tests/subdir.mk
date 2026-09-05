CC := g++

RSYSLOG-PLUGIN-TEST_OBJS += ./rsyslog_plugin_tests/rsyslog_plugin_ut.o

C_DEPS += ./rsyslog_plugin_tests/rsyslog_plugin_ut.d

rsyslog_plugin_tests/%.o: rsyslog_plugin_tests/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(CC) -D__FILENAME__="$(subst rsyslog_plugin_tests/,,$<)" $(CFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
