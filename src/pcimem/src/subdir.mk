CC := gcc

C_SRCS += \
../src/pcimem.c

OBJS += \
./src/pcimem.o

C_DEPS += \
./src/pcimem.d

src/%.o: ../src/%.c
	$(CC) -O3 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
