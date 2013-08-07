GCC = nspire-gcc
AR = arm-none-eabi-ar
GCCFLAGS = -Os -Wall -W -nostdlib -s

OBJCOPY := "$(shell which arm-elf-objcopy 2>/dev/null)"
ifeq (${OBJCOPY},"")
	OBJCOPY := arm-none-eabi-objcopy
endif
OBJS = adler32.o crc32.o \
	inffast.o inflate.o inftrees.o zutil.o
DISTDIR = .
vpath %.tns $(DISTDIR)

all: $(OBJS)
	$(AR) rcs libz.a $(OBJS)

%.o: %.c
	$(GCC) $(GCCFLAGS) -c $<

clean:
	rm -f *.o *.elf
	rm -f libz.a
