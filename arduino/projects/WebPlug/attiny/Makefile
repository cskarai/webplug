CC=/usr/bin/avr-gcc

CFLAGS=-g -Wall -Os -mcall-prologues -mmcu=attiny84 -DF_CPU=8000000 -fshort-enums -ffunction-sections -fdata-sections
LDFLAGS=$(COMMON_FLAGS) -lm -Wl,--relax,--gc-sections

LDFLAGS=-lm
OBJ2HEX=/usr/bin/avr-objcopy 
OBJDUMP=/usr/bin/avr-objdump
TARGET=attiny_halo

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

all : $(TARGET).src $(TARGET).hex codegen

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.obj : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	
%.src : %.obj
	$(OBJDUMP) -S --demangle $< >$@
	$(OBJDUMP) -EB -s $< >>$@
#	$(OBJDUMP) -EB -s -j .data $< >>$@

%.hex : %.obj
	$(OBJ2HEX) -R .eeprom -O ihex $< $@

clean :
	rm -f *.hex *.obj *.o *.src attiny_halo.h

codegen: $(TARGET).hex
	./generateCFromHex.pl
