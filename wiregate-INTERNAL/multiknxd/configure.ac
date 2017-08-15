dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT(multiknxd, 0.1, devel@wiregate.de)

AC_CHECK_LIB(eibclient,EIB_Cache_LastUpdates,,[AC_MSG_ERROR([eibclient not found or too old])])
AC_CHECK_LIB(json,json_tokener_parse,,[AC_MSG_ERROR([libjson0 (json-c) not found])])
AC_CHECK_LIB(pthread, pthread_create)

AC_ARG_WITH(debug, [  --with-debug            enable full debugging (default on)])

if test "$with_debug" == "yes"
then
	AC_DEFINE(DEBUG,[1],[Enable debugging])
fi

AC_CHECK_HEADER(argp.h,,[AC_MSG_ERROR([argp_parse not found])])
AC_SEARCH_LIBS(argp_parse,argp,,[AC_MSG_ERROR([argp_parse not found])])

dnl ############################################################
dnl Check libow
AC_ARG_ENABLE([owfs],
	AC_HELP_STRING([--enable-owfs], [build with ownet / 1-Wire support (default=auto)]),
	,
	enable_owfs=auto)
 
if test x"$enable_owfs" != xno ; then
	AC_CHECK_HEADER([ownetapi.h],
			enable_owfs=yes
			AC_DEFINE(ENABLE_OWFS,[1],[with owfs]),
			enable_owfs=no
	)
	AC_CHECK_LIB(ownet,OWNET_init)
fi
dnl ############################################################

dnl ############################################################
dnl Check libmodbus
AC_ARG_ENABLE([modbus],
	AC_HELP_STRING([--enable-modbus], [build with libmodbus support (default=auto)]),
	,
	enable_modbus=auto)
 
if test x"$enable_modbus" != xno ; then
	AC_CHECK_HEADER([modbus/modbus.h],
			enable_modbus=yes
			AC_DEFINE(ENABLE_MODBUS,[1],[with modbus]),
			enable_modbus=no
	)
	AC_CHECK_LIB(modbus,modbus_connect)
fi
dnl ############################################################


AC_CONFIG_HEADERS([config.h])
cp confdefs.h my_config.h

AM_INIT_AUTOMAKE([1.11])

AM_SILENT_RULES([yes])

AC_PROG_CC

dnl ***************************************************************************
dnl Internationalization
dnl ***************************************************************************
IT_PROG_INTLTOOL([0.35.0])

GETTEXT_PACKAGE=multiknxd
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

AM_GLIB_GNU_GETTEXT



LT_INIT






AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
doc/Makefile
iniparser/Makefile
])
