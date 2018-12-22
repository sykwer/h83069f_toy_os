PREFIX  = /usr/local/h8-elf
ARCH    = h8300-elf
BINDIR  = $(PREFIX)/bin
ADDNAME = $(ARCH)-

AR      = $(BINDIR)/$(ADDNAME)ar
AS      = $(BINDIR)/$(ADDNAME)as
CC      = $(BINDIR)/$(ADDNAME)gcc
LD      = $(BINDIR)/$(ADDNAME)ld
NM      = $(BINDIR)/$(ADDNAME)nm
OBJCOPY = $(BINDIR)/$(ADDNAME)objcopy
OBJDUMP = $(BINDIR)/$(ADDNAME)objdump
RANLIB  = $(BINDIR)/$(ADDNAME)ranlib
STRIP   = $(BINDIR)/$(ADDNAME)strip

H8WRITE = ../../../bin/tool/h8write

# FreeBSD-4.x:/dev/cuaaX, FreeBSD-6.x:/dev/cuadX, FreeBSD(USB):/dev/cuaUx
# Linux:/dev/ttySx, Linux(USB):/dev/ttyUSBx, Windows:comX
H8WRITE_SERDEV = /dev/cu.Repleo-PL2303-00001014

TARGET = ../../bin/bootload/kozos
OBJDIR = ../../bin/bootload/obj

SOURCES = $(wildcard *.c)
ASMS = startup.S intr.S

OBJS = $(addprefix $(OBJDIR)/, $(SOURCES:.c=.o))
OBJS += $(addprefix $(OBJDIR)/asm/, $(ASMS:.S=.o))

CFLAGS = -Wall -mh -nostdinc -nostdlib -fno-builtin
#CFLAGS += -mint32 # If changed to 32bit, you cannot multipy / divide int
CFLAGS += -I.
#CFLAGS += -g
CFLAGS += -Os
CFLAGS += -DKZLOAD

LFLAGS = -static -T script.ld -L.

.PHONY: all
all :		$(TARGET)

$(TARGET) :	$(OBJS)
		$(CC) $(OBJS) -o $(TARGET) $(CFLAGS) $(LFLAGS)
		cp $(TARGET) $(TARGET).elf
		$(STRIP) $(TARGET)

$(OBJDIR)/asm/%.o : asm/%.S
		$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(OBJDIR)/%.o : %.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<


$(TARGET).mot :	$(TARGET)
		$(OBJCOPY) -O srec $(TARGET) $(TARGET).mot

image :		$(TARGET).mot

write :		$(TARGET).mot
		$(H8WRITE) -3069 -f20 $(TARGET).mot $(H8WRITE_SERDEV)

.PHONY: clean
clean :
		rm -f $(OBJS) $(TARGET) $(TARGET).elf $(TARGET).mot
