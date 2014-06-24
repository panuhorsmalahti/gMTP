#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([gmtp], [1.3.6], [chewy509@mailcity.com])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_AUX_DIR([config])
AC_CANONICAL_HOST

AC_SUBST(ARCH,[host_cpu])

AM_INIT_AUTOMAKE([1.10 no-define foreign])
# Checks for programs.
AC_PROG_CC([gcc cc])
AC_PROG_CC_C99
AC_PROG_INSTALL
AC_PROG_LN_S
AM_GNU_GETTEXT_VERSION([0.16.1])
AM_GNU_GETTEXT([external])

# Checks for header files.
AC_CHECK_HEADERS([libintl.h locale.h stdlib.h string.h strings.h unistd.h libgen.h sys/types.h sys/stat.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_CHECK_HEADER_STDBOOL
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([mkdir strchr])

# Some common needed definitions.
AC_DEFINE([GMTP_GSETTINGS_SCHEMA], ["org.gnome.gmtp"], [GCONF Schema])
AC_DEFINE([_STDC_C99], [], [Ensure libraries are C99])
AC_DEFINE([_POSIX_C_SOURCE], [200112L], [Set POSIX Level])
AC_DEFINE([_XOPEN_SOURCE], [600], [Set XOPEN Source Level])

#AC_ARG_ENABLE([nls], AS_HELP_STRING([--disable-nls], [Disable NLS]), AC_DEFINE([ENABLE_NLS], [1] , AC_DEFINE([ENABLE_NLS], [0])

# Checks for libraries.
AC_ARG_WITH([gtk3], AS_HELP_STRING([--with-gtk3], [Build using GTK+ 3.0 (default)]), [], [with_gtk3=yes])
AC_ARG_WITH([gtk2], AS_HELP_STRING([--with-gtk2], [Build using GTK+ 2.0]), [], [with_gtk2=no])

AS_IF([test "x$with_gtk2" = "xno"] ,[
	AS_IF([test "x$with_gtk3" = "xyes"], [
		PKG_CHECK_MODULES([GTK3], [gtk+-3.0 >= 3.0.0], 
			[AC_DEFINE([HAVE_GTK3], [1], [Define if using GTK+3]) 
				PKG_CHECK_MODULES([GIO], [gio-2.0])
				GLIB_GSETTINGS 
				GCONF_SCHEMAS_INSTALL_FALSE='#'
				GCONF_SCHEMAS_INSTALL_TRUE=
				],
			[AC_DEFINE([HAVE_GTK3], [0], [Define if using GTK+3])
				# GTK3 not found, try GTK2
				PKG_CHECK_MODULES([GTK2], [gtk+-2.0 >= 2.4.9])
				PKG_CHECK_MODULES([GCONF], [gconf-2.0])
				AM_GCONF_SOURCE_2
				AC_PATH_PROG(GCONFTOOL, gconftool-2)

			])
	])
])

AS_IF([test "x$with_gtk2" = "xyes"], [
	AC_DEFINE([HAVE_GTK3], [0], [Define if using GTK+3])
	PKG_CHECK_MODULES([GTK2], [gtk+-2.0 >= 2.4.9])
	PKG_CHECK_MODULES([GCONF], [gconf-2.0])
	AM_GCONF_SOURCE_2
	AC_PATH_PROG(GCONFTOOL, gconftool-2)
])

# Common libraries
PKG_CHECK_MODULES([GTHREAD], [gthread-2.0])
PKG_CHECK_MODULES([LIBMTP], [libmtp >= 1.1.6])
PKG_CHECK_MODULES([FLAC], [flac])
PKG_CHECK_MODULES([ID3TAG], [id3tag])
PKG_CHECK_MODULES([VORBISFILE], [vorbisfile])

AC_CONFIG_FILES([
Makefile src/Makefile 
images/Makefile 
po/Makefile.in 
misc/gmtp.desktop
misc/org.gnome.gmtp.gschema.xml
misc/Makefile])
AC_OUTPUT
