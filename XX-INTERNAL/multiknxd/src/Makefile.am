## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" 

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = multiknxd

multiknxd_SOURCES = \
	main.c \
	daemon.c \
	daemon.h \
	globals.h \
	utils.c \
	utils.h \
	owhandler.c \
	owhandler.h \
	knxhandler.c \
	knxhandler.h

multiknxd_LDFLAGS = 

multiknxd_LDADD = ../iniparser/iniparser.o ../iniparser/dictionary.o

