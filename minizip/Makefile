GCC = nspire-gcc
AR = arm-none-eabi-ar
GCCFLAGS = -Os -Wall -W -nostdlib -s -I../libz
 
OBJCOPY := "$(shell which arm-elf-objcopy 2>/dev/null)"
ifeq (${OBJCOPY},"")
	OBJCOPY := arm-none-eabi-objcopy
endif
OBJS = ioapi.o unzip.o
DISTDIR = .
vpath %.tns $(DISTDIR)

all: $(OBJS)
	$(AR) rcs libunzip.a ../libz/adler32.o ../libz/crc32.o \
	../libz/inffast.o ../libz/inflate.o ../libz/inftrees.o ../libz/zutil.o \
	$(OBJS)

%.o: %.c
	$(GCC) $(GCCFLAGS) -c $<

clean:
	rm -f *.o *.elf
	rm -f libunzip.a