bootblock: boot/bootblock.o
	$(OBJCOPY) -S -O binary -j .text $< $@
	./boot/sign.pl $@

boot/bootblock.o: boot/bootasm.o boot/bootmain.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o $@ $^

boot/bootmain.o: boot/bootmain.c
	$(CC) $(CFLAGS) -O -c $< -o $@

boot/bootasm.o: boot/bootasm.S
	$(CC) $(CFLAGS) -c $< -o $@
