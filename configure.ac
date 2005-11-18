# Process this file with autoconf to produce a configure script.
AC_INIT(src)
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([gmpc], [0.12.1],qball@qballcow.nl)

# Checks for programs.

AC_PROG_INTLTOOL([0.21])
AC_PROG_LIBTOOL
AC_PROG_CC

AC_ARG_ENABLE(mmkeys,[  --disable-mmkeys     Disable multimedia keys support.],,enable_mmkeys=yes)
AC_ARG_ENABLE(trayicon,[  --disable-trayicon     Disable tray icon support.],,enable_trayicon=yes)

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

AC_SUBST(GLIB_REQUIRED)
AC_SUBST(GTK_REQUIRED)


PKG_CHECK_MODULES([gmpc],
[
	glib-2.0 >= 2.4
	gobject-2.0 >= 2.4
	gtk+-2.0 >= 2.4
	libglade-2.0
	libxml-2.0
	gmodule-2.0
	libmpd >= 0.0.9.15
])
AC_SUBST(gmpc_LIBS)
AC_SUBST(gmpc_CFLAGS)


if test x${enable_mmkeys} = xyes; then
	AC_DEFINE(ENABLE_MMKEYS, 1, [Enable multimedia support])
fi

if test x${enable_trayicon} = xyes; then
	AC_DEFINE(ENABLE_TRAYICON, 1, [Enable tray icon support])
fi





# installation paths
AC_MSG_CHECKING(prefix)

if test "x${prefix}" = "xNONE"; then
	PACKAGE_PREFIX="${ac_default_prefix}"
else
	PACKAGE_PREFIX="${prefix}"
fi
AC_MSG_RESULT($PACKAGE_PREFIX)

# i18n support
dnl please keep them in alphabetical order
ALL_LINGUAS="fr ru"

GETTEXT_PACKAGE=gmpc
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",[GMPC gettext package])
AM_GLIB_GNU_GETTEXT


#AM_GNU_GETTEXT([external])
# setting correct paths
PACKAGE_LOCALE_DIR="${PACKAGE_PREFIX}/${DATADIRNAME}/locale"
AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR,"$PACKAGE_LOCALE_DIR",[The package's locale path for gettext])


AC_SUBST(PACKAGE_LOCALE_DIR)
AC_SUBST(PACKAGE_LIBS)

EXTRA_CFLAGS=
EXTRA_LDFLAGS=
case $host in
  *-*-mingw32* | *-*-windows)
    EXTRA_CFLAGS="-mno-cygwin -mwindows -mms-bitfields"
    EXTRA_LDFLAGS="-export-all-symbols -mwindows -mms-bitfields" ;;
  *-*-cygwin*)
    EXTRA_LDFLAGS="-export-all-symbols -mms-bitfields" ;;
esac
AC_SUBST(EXTRA_CFLAGS)
AC_SUBST(EXTRA_LDFLAGS)

AC_OUTPUT(Makefile src/Makefile pixmaps/Makefile glade/Makefile data/Makefile po/Makefile.in)
echo ""
echo ""
echo "-------------- Status --------------"
if test x${enable_mmkeys} = xyes; then
	echo "Multimedia keys support is:  	enabled"
else
	echo "Multimedia keys support is:  	disabled"
fi
if test x${enable_trayicon} = xyes; then
	echo "Tray icon support is:         	enabled"
else
	echo "Tray icon support is:	   	disabled"
fi
