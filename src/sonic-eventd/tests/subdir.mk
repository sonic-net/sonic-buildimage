CC := g++

TEST_OBJS += ./tests/eventd_ut.o ./tests/main.o

C_DEPS += ./tests/eventd_ut.d ./tests/main.d

tests/%.o: tests/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(CC) -D__FILENAME__="$(subst tests/,,$<)" $(CFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
