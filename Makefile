CC         := arm-none-eabi-gcc
COMFLAGS   := -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb --specs=nosys.specs
DEVFLAGS   := -DSTM32H725xx -ffunction-sections -fdata-sections -fstack-usage
CFLAGS     := -std=gnu11 -DDEBUG -g3 -ggdb3 -O0 -Wall -Werror -Wextra -Wconversion -Wpedantic -pedantic-errors
LFLAGS     := -static -Wl,-Map=ecu.map -Wl,--gc-sections
INCLUDE    := -I./vendor/stm32cube/Drivers/CMSIS/Device/ST/STM32H7xx/Include -I./vendor/stm32cube/Drivers/CMSIS/Include
OBJS       := ecu.o system_stm32h7xx.o startup_stm32h725xx.o

#TODO: -mslow-flash-data (others?)

.PHONY: def
def : ecu.elf

.PHONY: clean
clean :
	rm -f *.o *.su *.elf *.map

%.o : %.c
	$(CC) $(CFLAGS) $(COMFLAGS) $(DEVFLAGS) $(INCLUDE) -c $< -o $@
ecu.o :

system_stm32h7xx.o : ./vendor/stm32cube/Drivers/CMSIS/Device/ST/STM32H7xx/Source/Templates/system_stm32h7xx.c
	$(CC) $(CFLAGS) $(COMFLAGS) $(DEVFLAGS) -Wno-sign-conversion $(INCLUDE) -c $< -o $@

startup_stm32h725xx.o : ./vendor/stm32cube/Drivers/CMSIS/Device/ST/STM32H7xx/Source/Templates/gcc/startup_stm32h725xx.s
	$(CC) -x assembler $(CFLAGS) $(COMFLAGS) $(DEVFLAGS) $(INCLUDE) -c $< -o $@

ecu.elf : $(OBJS)
	$(CC) -o $@ -T"./STM32H725VGTX_FLASH.ld" $(COMFLAGS) $(LFLAGS) $(OBJS)
