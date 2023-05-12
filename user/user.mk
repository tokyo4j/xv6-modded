ULIB_SRCS := $(wildcard user/ulib/*)
ULIB_OBJS := $(patsubst %.c, %.o,\
							$(patsubst %.S, %.o,\
					      $(ULIB_SRCS)))

UPROG_SRCS = $(wildcard user/*.c user/*.S)

UPROGS = $(patsubst user/%, user/_%, $(basename $(UPROG_SRCS)))

.PRECIOUS: $(addsuffix .o, $(basename $(UPROG_SRCS)))

user/_forktest: user/forktest.o user/ulib/ulib.o user/ulib/usys.o
# forktest has less library code linked in - needs to be small
# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -Tuser/user.ld -o $@ $^

user/_%: user/%.o $(ULIB_OBJS)
	$(LD) $(LDFLAGS) -Tuser/user.ld -o $@ $^

user/_usertests: user/usertests.o $(ULIB_OBJS)
	$(LD) $(LDFLAGS) -Tuser/user.ld -o $@_debug $^
	strip --strip-debug $@_debug -o $@
