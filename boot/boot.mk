bootblock: boot/bootblock.o boot/sign.pl
	$(OBJCOPY) -S -O binary -j .text $< $@
	./boot/sign.pl $@

boot/bootblock.o: boot/bootasm.o boot/bootmain.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o $@ $^

ifeq ($(USE_CLANG), y)
OPT_FOR_SIZE = -Oz
else
OPT_FOR_SIZE = -O
endif

boot/%.o: boot/%.c
	$(CC) $(CFLAGS) $(OPT_FOR_SIZE) -c $< -o $@
