# Process this file with autoconf to produce a configure script.
AC_INIT(src)
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE(gmpc, 0.12.1,qball@qballcow.nl)

# Checks for programs.

AC_PROG_INTLTOOL([0.21])
AC_PROG_LIBTOOL
AC_PROG_CC

AC_ARG_ENABLE(gnome-vfs,[  --disable-gnome-vfs     disable extra options that use gnome-vfs.],,enable_gnome_vfs=yes)
AC_ARG_ENABLE(libnotify,[  --enable-libnotify     disable extra options that use gnome-vfs.],,enable_libnotify=no)


# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

AC_SUBST(GLIB_REQUIRED)
AC_SUBST(GTK_REQUIRED)


PKG_CHECK_MODULES(gmpc,
[
	glib-2.0 >= 2.4
	gobject-2.0 >= 2.4
	gtk+-2.0 >= 2.4
	libglade-2.0
	libxml-2.0
	gmodule-2.0
	libmpd >= 0.0.9
])
AC_SUBST(gmpc_LIBS)
AC_SUBST(gmpc_CFLAGS)

if test x${enable_gnome_vfs} = xyes; then
	PKG_CHECK_MODULES(gmpc_gvfs, [gnome-vfs-2.0])
	AC_DEFINE(ENABLE_GNOME_VFS, 1, [Enable extra options that use gnome-vfs])
fi

if test x${enable_libnotify} = xyes; then
	PKG_CHECK_MODULES(gmpc_libnotify, [libnotify])
	AC_DEFINE(ENABLE_LIBNOTIFY, 1, [Enable extra options that use gnome-vfs])
fi




AC_SUBST(gmpc_gvfs_LIBS)
AC_SUBST(gmpc_gvfs_CFLAGS)

AC_SUBST(gmpc_libnotify_LIBS)
AC_SUBST(gmpc_libnotify_CFLAGS)

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


PIXMAP_PATH="${PACKAGE_PREFIX}/${DATADIRNAME}/${PACKAGE}/";

AC_DEFINE_UNQUOTED(GLADE_PATH,"$PIXMAP_PATH",[The package's glade path])
AC_DEFINE_UNQUOTED(PIXMAP_PATH,"$PIXMAP_PATH",[The package's pixmap path])

AC_SUBST(PACKAGE_LOCALE_DIR)
AC_SUBST(PACKAGE_LIBS)

AC_OUTPUT(Makefile src/Makefile pixmaps/Makefile glade/Makefile data/Makefile po/Makefile.in)
echo ""
echo ""
echo "-------------- Status --------------"
if test x${enable_gnome_vfs} = xyes; then
	echo "Gnome VFS support is:	 	enabled."
else
	echo "Gnome VFS support is:		disabled."
fi
if test x${enable_libnotify} = xyes; then
	echo "Gnome Notification support is: 	enabled."
else
	echo "Gnome Notification support is:	disabled."
fi
