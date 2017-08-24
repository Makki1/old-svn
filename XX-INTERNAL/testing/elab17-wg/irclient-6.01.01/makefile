SHELL = /bin/sh

CC = gcc

CFLAGS = 
CPPFLAGS = -DLINUX 
LDFLAGS =
LIBS =
ODIR = x86
ODIR64 = x64
ODIRARM = arm

_OBJS = client.o
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))
OBJS64 = $(patsubst %,$(ODIR64)/%,$(_OBJS))
OBJSARM = $(patsubst %,$(ODIRARM)/%,$(_OBJS))


irclient: $(OBJS) 
	$(CC) $(CFLAGS) $(OBJS) -m32 -o irclient $(LDFLAGS)

irclient64: $(OBJS64)
	$(CC) $(CFLAGS) -DX64 $(OBJS64) -m64 -o irclient64 $(LDFLAGS)

irclient_arm: $(OBJSARM)
	$(CC) $(CFLAGS) $(OBJSARM) -o irclient $(LDFLAGS)


all: irclient irclient64

arm: irclient_arm


clean:
	-rm $(OBJS)
	-rm $(OBJS64)
	-rm $(OBJSARM)


$(ODIR)/%.o: %.c flash.h errcode.h network.h remote.h makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -m32 -c $< -o $@


$(ODIR64)/%.o: %.c flash.h errcode.h network.h remote.h makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -DX64 -m64 -c $< -o $@

$(ODIRARM)/%.o: %.c  flash.h errcode.h network.h remote.h makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

