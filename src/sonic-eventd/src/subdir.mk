CC := g++

TEST_OBJS += ./src/eventd.o
OBJS += ./src/eventd.o ./src/main.o
EVENTDB_OBJS += ./src/eventdb.o ./src/eventconsume.o ./src/loghandler.o ./src/eventutils.o

C_DEPS += ./src/eventd.d ./src/main.d ./src/eventdb.d ./src/eventconsume.d ./src/loghandler.d ./src/eventutils.d

src/%.o: src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(CC) -D__FILENAME__="$(subst src/,,$<)" $(CFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
