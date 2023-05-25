CC := g++

TEST_OBJS += ./src/eventd.o
OBJS += ./src/eventd.o ./src/main.o

C_DEPS += ./src/eventd.d ./src/main.d

src/%.o: src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(CC) -D__FILENAME__="$(subst src/,,$<)" $(CFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
