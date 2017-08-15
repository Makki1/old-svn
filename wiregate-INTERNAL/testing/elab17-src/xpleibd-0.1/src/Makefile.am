bin_PROGRAMS = xpleibd
xpleibd_SOURCES = xpleibd.c common.c

# set the include path found by configure
INCLUDES= $(all_includes)

# the library search path.
xpleibd_LDFLAGS = $(all_libraries) $(LIBEIBCLIENT) $(LIBXPL)
