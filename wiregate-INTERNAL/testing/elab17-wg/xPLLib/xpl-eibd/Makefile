#
# Makefile for xpl-eibd
#

#
# For LINUX, use the following
CCOPTS = -g -DLINUX -pedantic -Wall
LIBS = -g -lm

# For UnixWare systems, use the following
# CCOPTS = -O2 -DUNIXWARE
# LIBS = -lnsl -lsocket -lm

#
# For Alpha/DEC OSF/Tru64
#
# CCOPTS = -O2 -DTRU64
# LIBS = -lm

# *******************************************************
# ******* No more customizations from here down *********
# *******************************************************
# 	$(LD) -o $@ $< ../xPL.a /usr/lib/libeibclient.a $(LIBS)

# LDOPTS	= -O -s = stripped!
LDOPTS	= -O
CC	= gcc $(CCOPTS)
LD	= gcc $(LDOPTS)

# define the list of source files.
SRCS = main.c file1.c file2.c
# most of the makefile remains as it was before.
# at the bottom, we add these lines:

# program executable file name.
PROG = xpl-eibd
# list of generated object files.
OBJS = xpl-eibd.o common.o

#	$(LD) -o $@ $< ../xPL.a /usr/lib/libeibclient.a $(LIBS)

$(PROG): $(OBJS)
	$(LD) $(LDOPTS) $(OBJS) -o $(PROG) ../xPL.a /usr/lib/libeibclient.a

# now comes a meta-rule for compiling any "C" source file.
%.o: %.c
	$(CC) $(CCOPTS) -c $<

all:	${PROG}

clean:
	rm -f *.o *.a core ${PROG}

rebuild: clean all

