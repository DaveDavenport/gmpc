AC_INIT([gmpc], [package_version], [qball@sarine.nl])
#Define the version
m4_define([package_major_version], [0])
m4_define([package_minor_version], [16])
m4_define([package_micro_version], [95])
m4_define([package_version], [package_major_version.package_minor_version.package_micro_version])


#Define variable
m4_define([package_tagline],       "♯♪ + ♭♪ = ☺")
m4_define([package_website],       "http://gmpcwiki.sarine.nl")
m4_define([package_copyright],     "Copyright 2003-2008 Qball Cow")
m4_define([package_bugtracker],    "http://gmpcwiki.sarine.nl/index.php/Help:Contents")



AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE


#Make version and variables available in config.h
AC_DEFINE(GMPC_MAJOR_VERSION,   package_major_version,	["Major version of gmpc"])
AC_DEFINE(GMPC_MINOR_VERSION,   package_minor_version,	["Minor version of gmpc"])
AC_DEFINE(GMPC_MICRO_VERSION,   package_micro_version,	["Micro version of gmpc"])
AC_DEFINE(GMPC_TAGLINE,         package_tagline,	["Gmpc's tagline"])
AC_DEFINE(GMPC_WEBSITE,		package_website,	["Gmpc's website"])
AC_DEFINE(GMPC_BUGTRACKER,      package_bugtracker,	["Gmpc's bugtracker"])
AC_DEFINE(GMPC_COPYRIGHT,       package_copyright,	["Gmpc's Copyright"])


AC_PREREQ(2.59)

# Remove the check for c++ and fortran compiler
m4_defun([_LT_AC_LANG_CXX_CONFIG], [:])
m4_defun([_LT_AC_LANG_F77_CONFIG], [:])


#intltool, libtool. Check for CC compiler
AC_PROG_INTLTOOL([0.21])

# Check for intltool version, needed for make dist. 
AM_CONDITIONAL([INTLTOOL_HIGHER_04000], [test ${INTLTOOL_APPLIED_VERSION_AS_INT} -ge 4000])


AC_PROG_CC
LT_AC_PROG_RC
AC_LIBTOOL_WIN32_DLL
AC_LIBTOOL_RC
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for libraries.
AC_SUBST(GLIB_REQUIRED)
AC_SUBST(GTK_REQUIRED)

##
# User params 
##

# Config dir support
AC_ARG_ENABLE(configdir,
        [  --enable-configdir		Use .config instead of .gmpc or .covers.],
        [ case "${enableval}" in
            yes)    enable_configdir=yes;;
             no)    enable_configdir=no;;
              *)    AC_MSG_ERROR([bad value ${enableval} for --enable-configdir]);;
          esac],
        [enable_configdir=no])

if test x${enable_configdir} = xyes; then
	AC_DEFINE(USE_CONFIG_DIR,1, [Use Config Dir])
fi

# Session support             
AC_ARG_ENABLE(sm,
              [  --disable-sm				Disable Session Support.],
              [ case "${enableval}" in
                    yes)    enable_sm=yes;;
                     no)    enable_sm=no;;
                      *)    AC_MSG_ERROR([bad value ${enableval} for --disable-dm]);;
                esac],
              [enable_sm=yes])

# sm
if test x${enable_sm} = xyes; then
	PKG_CHECK_MODULES([sm], sm)
	AC_SUBST(sm_LIBS)
	AC_SUBST(sm_CFLAGS)
	AC_DEFINE(ENABLE_SM, 1, [Enable Session Support])

fi
# Multimedia keys
AC_ARG_ENABLE([mmkeys],
              [  --disable-mmkeys			Disable multimedia keys support.],
              [ case "${enableval}" in
                    yes)    enable_mmkeys=yes;;
                     no)    enable_mmkeys=no;;
                      *)    AC_MSG_ERROR([bad value ${enableval} for --disable--mmkeys]);;
                esac],
              [enable_mmkeys=yes])
if test x${enable_mmkeys} = xyes; then
	AC_DEFINE(ENABLE_MMKEYS, 1, [Enable multimedia support])
fi


# Maintainer mode
AC_ARG_ENABLE([timing],
              [--enable-timing Print timing debug output.],
	          [case "${enableval}" in
		            yes)    enable_timing=true;;
		             no)    enable_timing=false;;
		              *)    AC_MSG_ERROR([bad value ${enableval} for --enable-timing]);;
               esac],
               [enable_timing=false])

if test x${enable_timing} = xtrue; then
# Add DEBUG_ENABLE define to config.h
    AC_DEFINE(DEBUG_TIMING, 1, [Enable timing output])
fi

# Maintainer mode
AC_ARG_ENABLE([maintainermode],
	[--enable-maintainermode Enable maintainer mode.],
	[case "${enableval}" in
		yes) maintainer_mode=true;;
		no)  maintainer_mode=false;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-maintainermode]);;
	 esac],
	[maintainer_mode=false])

AM_CONDITIONAL([MAINTAINER_MODE], [test x$maintainer_mode = xtrue])


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
PKG_CHECK_MODULES([gtk], gtk+-2.0 >= 2.12)
AC_SUBST(gtk_LIBS)
AC_SUBST(gtk_CFLAGS)

#gmodule
PKG_CHECK_MODULES([gmodule], gmodule-2.0 >= 2.4)
AC_SUBST(gmodule_LIBS)
AC_SUBST(gmodule_CFLAGS)

# libmpd
PKG_CHECK_MODULES([libmpd], libmpd >= 0.16.2)
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
ALL_LINGUAS="bg cs de es fr gl he hu it nl nb pl ro ru sv tr zh_CN"

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


#Win32 compile support

EXTRA_CFLAGS=
EXTRA_LDFLAGS=
case "$CC" in
*gcc*)
	EXTRA_CFLAGS="-Wmissing-prototypes -Wmissing-declarations"
	;;
esac

win32=no
macosx=no
case $host in
  *-*-mingw32* | *-*-windows)
    win32=yes
    EXTRA_CFLAGS="$EXTRA_CFLAGS -mno-cygwin -mwindows -mms-bitfields"
    EXTRA_LDFLAGS="-export-all-symbols -mwindows -mms-bitfields -lregex" ;;
  *-*-cygwin*)
    win32=yes
    EXTRA_LDFLAGS="-export-all-symbols -mms-bitfields" ;;
  *-apple-darwin*)
    macosx=yes;;
esac
AM_CONDITIONAL(WIN32, test x$win32 = xyes)
AM_CONDITIONAL(OSX, test x$macosx = xyes)
if test x$macosx = xyes; then
# mac integration 
PKG_CHECK_MODULES([macint],ige-mac-integration)
AC_SUBST(macint_CFLAGS)
AC_SUBST(macint_LIBS)
fi

AC_SUBST(EXTRA_CFLAGS)
AC_SUBST(EXTRA_LDFLAGS)

# Check for gob2
GOB2_CHECK(2.0.0)
# Make the gob2 check fatal
if test x${GOB2} = x""; then
	exit 0;
fi


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
src/gob/Makefile
glade/Makefile
data/Makefile
po/Makefile.in
po/Makefile
data/gmpc.pc
remote/Makefile
doc/Makefile
pixmaps/Makefile
pixmaps/icons/Makefile
pixmaps/icons/16x16/Makefile
pixmaps/icons/16x16/actions/Makefile
pixmaps/icons/16x16/status/Makefile
pixmaps/icons/16x16/apps/Makefile
pixmaps/icons/22x22/Makefile
pixmaps/icons/22x22/actions/Makefile
pixmaps/icons/22x22/status/Makefile
pixmaps/icons/22x22/apps/Makefile
pixmaps/icons/32x32/Makefile
pixmaps/icons/32x32/actions/Makefile
pixmaps/icons/32x32/status/Makefile
pixmaps/icons/32x32/apps/Makefile
pixmaps/icons/48x48/Makefile
pixmaps/icons/48x48/actions/Makefile
pixmaps/icons/48x48/apps/Makefile
pixmaps/icons/scalable/Makefile
pixmaps/icons/scalable/actions/Makefile
pixmaps/icons/scalable/status/Makefile
pixmaps/icons/scalable/apps/Makefile
])
echo ""
echo ""
echo "------------------ Status ------------------"
if test x$maintainer_mode = xtrue; then
	echo "Maintainer mode is:               enabled"
else
	echo "Maintainer mode is:               disabled"
fi
if test x$enable_timing = xtrue; then
	echo "Debug timing output is:           enabled"
else
	echo "Debug timing output is:           disabled"
fi
if test x${enable_mmkeys} = xyes; then
	echo "Multimedia keys support is:       enabled"
else
	echo "Multimedia keys support is:       disabled"
fi
if test x${enable_sm} = xyes; then
	echo "Session Manager support is:       enabled"
else
	echo "Session Manager support is:       disabled"
fi
if test x${enable_configdir} = xyes; then
	echo "Use ~/.config/ for config files:  enabled"
else
	echo "Use ~/.config/ for config files:  disabled"
fi


echo "Now type make to build"
