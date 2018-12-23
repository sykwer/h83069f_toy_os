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

TARGET = ../../bin/os/kozos
OBJDIR = ../../bin/os/obj

SOURCES = $(wildcard *.c)
SOURCES += $(wildcard system_task/*.c)
SOURCES += $(wildcard user_task/*.c)
ASMS = startup.s dispatch.s

OBJS = $(addprefix $(OBJDIR)/, $(SOURCES:.c=.o))
OBJS += $(addprefix $(OBJDIR)/asm/, $(ASMS:.s=.o))

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

$(OBJDIR)/asm/%.o : asm/%.s
		$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(OBJDIR)/%.o : %.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(OBJDIR)/system_task/%.o : system_task/$.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(OBJDIR)/user_task/%.o : user_task/$.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

.PHONY: clean
clean :
		rm -f $(OBJS)
