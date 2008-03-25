# Process this file with autoconf to produce a configure script.
m4_define([gmpc_major_version], [0])
m4_define([gmpc_minor_version], [15])
m4_define([gmpc_micro_version], [5])
m4_define([gmpc_nano_version], [2])
m4_define([gmpc_version],
          [gmpc_major_version.gmpc_minor_version.gmpc_micro_version.gmpc_nano_version])

AC_INIT(src)
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([gmpc], [gmpc_version],qball@sarine.nl)


GMPC_MAJOR_VERSION=gmpc_major_version
GMPC_MINOR_VERSION=gmpc_minor_version
GMPC_MICRO_VERSION=gmpc_micro_version
GMPC_NANO_VERSION=gmpc_nano_version
AC_DEFINE(GMPC_MAJOR_VERSION, gmpc_major_version, ["Major version of gmpc"])
AC_DEFINE(GMPC_MINOR_VERSION, gmpc_minor_version, ["Minor version of gmpc"])
AC_DEFINE(GMPC_MICRO_VERSION, gmpc_micro_version, ["Micro version of gmpc"])
AC_DEFINE(GMPC_NANO_VERSION, gmpc_nano_version, ["Nano version of gmpc"])


# Checks for programs.

AC_PREREQ(2.59)
AC_PROG_INTLTOOL([0.21])
AC_PROG_LIBTOOL
AC_PROG_CC
AC_LIBTOOL_RC

AM_CONDITIONAL(HAVE_RC, test "x$RC" != "xno")

AC_ARG_ENABLE(configdir,    
              [  --enable-configdir		Use .config instead of .gmpc or .covers.],  
               [ if test "x$enableval" = "xyes"; then              
                enable_configdir=yes                           
            else                                                   
                enable_configdir=no                            
            fi],                                                   
              enable_configdir=no)
AC_ARG_ENABLE(trayicon,
              [  --disable-trayicon  Disable the tray icon.],         
              [ if test "x$enableval" = "xyes"; then 
               enable_trayicon=yes
           else
               enable_trayicon=no
           fi],               
              enable_trayicon=yes)
AC_ARG_ENABLE(sm,		    
              [  --disable-sm				Disable Session Support.],                 
              [ if test "x$enableval" = "xyes"; then 
               enable_sm=yes
           else
               enable_sm=no
           fi],               
              enable_sm=yes)
AC_ARG_ENABLE(mmkeys,	    
              [  --disable-mmkeys			Disable multimedia keys support.],          
              [ if test "x$enableval" = "xyes"; then 
                    enable_mmkeys=yes
                else
                    enable_mmkeys=no
                fi],           
              enable_mmkeys=yes)


# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])
AC_CHECK_HEADER([regex.h], [],
                [AC_MSG_ERROR([Required header regex.h not found.])])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

AC_SUBST(GLIB_REQUIRED)
AC_SUBST(GTK_REQUIRED)

# Split this out, because pkgconfig macro doesn't return nicely what is missing

# glib
PKG_CHECK_MODULES([glib], glib-2.0 >= 2.10)
AC_SUBST(glib_LIBS)
AC_SUBST(glib_CFLAGS)

# gobject
PKG_CHECK_MODULES([gobject], gobject-2.0 >= 2.4)
AC_SUBST(gobject_LIBS)
AC_SUBST(gobject_CFLAGS)

#gtk
PKG_CHECK_MODULES([gtk], gtk+-2.0 >= 2.10)
AC_SUBST(gtk_LIBS)
AC_SUBST(gtk_CFLAGS)

#gmodule
PKG_CHECK_MODULES([gmodule], gmodule-2.0 >= 2.4)
AC_SUBST(gmodule_LIBS)
AC_SUBST(gmodule_CFLAGS)

# libmpd
PKG_CHECK_MODULES([libmpd], libmpd >= 0.15.2)
AC_SUBST(libmpd_LIBS)
AC_SUBST(libmpd_CFLAGS)

# libglade
PKG_CHECK_MODULES([libglade], libglade-2.0)
AC_SUBST(libglade_LIBS)
AC_SUBST(libglade_CFLAGS)

# gthread
PKG_CHECK_MODULES([gthread], gthread-2.0)
AC_SUBST(gthread_LIBS)
AC_SUBST(gthread_CFLAGS)


# sm
if test x${enable_sm} = xyes; then
	PKG_CHECK_MODULES([sm], sm)
	AC_SUBST(sm_LIBS)
	AC_SUBST(sm_CFLAGS)
	AC_DEFINE(ENABLE_SM, 1, [Enable Session Support])

fi

if test x${enable_mmkeys} = xyes; then
	AC_DEFINE(ENABLE_MMKEYS, 1, [Enable multimedia support])
fi
if test x${enable_configdir} = xyes; then
	AC_DEFINE(USE_CONFIG_DIR,1, [Use Config Dir])
fi
if test x${enable_trayicon} = xyes; then
	AC_DEFINE(ENABLE_TRAYICON,1, [Disable Tray Icon])
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
ALL_LINGUAS="de fr gl it nl nb_NO pl ru"

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
case "$CC" in
*gcc*)
	EXTRA_CFLAGS="-Wmissing-prototypes -Wmissing-declarations"
	;;
esac

win32=no
case $host in
  *-*-mingw32* | *-*-windows)
    win32=yes
    EXTRA_CFLAGS="$EXTRA_CFLAGS -mno-cygwin -mwindows -mms-bitfields"
    EXTRA_LDFLAGS="-export-all-symbols -mwindows -mms-bitfields" ;;
  *-*-cygwin*)
    win32=yes
    EXTRA_LDFLAGS="-export-all-symbols -mms-bitfields" ;;
esac
AM_CONDITIONAL(WIN32, test x$win32 = xyes)

AC_SUBST(EXTRA_CFLAGS)
AC_SUBST(EXTRA_LDFLAGS)
GOB2_CHECK(2.0.0)


# libcurl settings
AC_PATH_PROG(CURL_CONFIG,curl-config)
AC_ARG_WITH(curl,
            AC_HELP_STRING([--with-curl=PATH], [where libcurl is installed]),
            [CURL_CONFIG="${with_curl}/bin/curl-config"])
if test -f "${CURL_CONFIG}"; then
    LDFLAGS="`${CURL_CONFIG} --libs` $LDFLAGS"
    CPPFLAGS="`${CURL_CONFIG} --cflags` $CPPFLAGS"
else
    AC_MSG_WARN([curl-config not found, guessing at libcurl build settings])
fi
AC_CHECK_HEADER([curl/curl.h],,AC_MSG_ERROR([unable to find libcurl header files]))
AC_CHECK_LIB([curl],[curl_global_init],,AC_MSG_ERROR([unable to link with libcurl]))


AC_OUTPUT([
Makefile 
src/Makefile 
glade/Makefile 
data/Makefile 
po/Makefile.in 
po/Makefile 
data/gmpc.pc 
doc/Makefile 
remote/Makefile
pixmaps/Makefile 
pixmaps/icons/Makefile
pixmaps/icons/16x16/Makefile
pixmaps/icons/16x16/actions/Makefile 
pixmaps/icons/22x22/Makefile
pixmaps/icons/22x22/actions/Makefile 
pixmaps/icons/32x32/Makefile
pixmaps/icons/32x32/actions/Makefile
pixmaps/icons/48x48/Makefile
pixmaps/icons/48x48/actions/Makefile
pixmaps/icons/scalable/Makefile
pixmaps/icons/scalable/actions/Makefile
])
echo ""
echo ""
echo "-------------- Status --------------"
if test x${enable_mmkeys} = xyes; then
	echo "Multimedia keys support is:  		enabled"
else
	echo "Multimedia keys support is:  		disabled"
fi
if test x${enable_sm} = xyes; then
	echo "Session Manager support is:		enabled"
else
	echo "Session Manager support is:		disabled"
fi
if test x${enable_trayicon} = xyes; then
	echo "Trayicon support is:		        enabled"
else
	echo "Trayicon support is:		        disabled"
fi
if test x${enable_configdir} = xyes; then
	echo "Use ~/.config/ for config files:	enabled"
else
	echo "Use ~/.config/ for config files:	disabled"
fi


echo "Now type make to build"
