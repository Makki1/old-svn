bin_PROGRAMS = xpl_eibd
xpl_eibd_SOURCES = eib_common.c xpl_eibd.c

# set the include path found by configure
INCLUDES= $(all_includes)

# the library search path.
xpl_eibd_LDFLAGS = $(all_libraries) 
noinst_HEADERS = eib_common.h
xpl_eibd_LDADD = -lxPL libeibclient
