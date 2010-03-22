#Define the version
m4_define([package_major_version], [0])
m4_define([package_minor_version], [20])
m4_define([package_micro_version], [0])
m4_define([package_version], [package_major_version.package_minor_version.package_micro_version])

AC_INIT([gmpc], [0.20.0], [qball@sarine.nl])

#Define variable
m4_define([package_tagline],       "  ")
m4_define([package_website],       "http://gmpc.wikia.com")
m4_define([package_copyright],     "Copyright 2003-2010 Qball Cow")
m4_define([package_bugtracker],    "http://gmpc.wikia.com/wiki/GMPC_HELP")



AM_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

AM_MAINTAINER_MODE


#Make version and variables available in config.h
#AC_DEFINE(GMPC_MAJOR_VERSION,   package_major_version,	["Major version of gmpc"])
#AC_DEFINE(GMPC_MINOR_VERSION,   package_minor_version,	["Minor version of gmpc"])
#AC_DEFINE(GMPC_MICRO_VERSION,   package_micro_version,	["Micro version of gmpc"])
# This makes sure the right substitution is done
[GMPC_MAJOR_VERSION]=package_major_version
[GMPC_MINOR_VERSION]=package_minor_version
[GMPC_MICRO_VERSION]=package_micro_version
AC_SUBST([GMPC_MAJOR_VERSION])
AC_SUBST([GMPC_MINOR_VERSION])
AC_SUBST([GMPC_MICRO_VERSION])
AC_DEFINE(GMPC_TAGLINE,         package_tagline,	["Gmpc's tagline"])
AC_DEFINE(GMPC_WEBSITE,		package_website,	["Gmpc's website"])
AC_DEFINE(GMPC_BUGTRACKER,      package_bugtracker,	["Gmpc's bugtracker"])
AC_DEFINE(GMPC_COPYRIGHT,       package_copyright,	["Gmpc's Copyright"])


AC_PREREQ(2.59)

# Remove the check for c++ and fortran compiler
m4_defun([_LT_AC_LANG_CXX_CONFIG], [:])
m4_defun([_LT_AC_LANG_F77_CONFIG], [:])
AC_REPLACE_FUNCS(strndup)


#intltool, libtool. Check for CC compiler
AC_PROG_INTLTOOL([0.21])

# Check for intltool version, needed for make dist. 
AM_CONDITIONAL([INTLTOOL_HIGHER_04000], [test ${INTLTOOL_APPLIED_VERSION_AS_INT} -ge 4000])


AC_PROG_CC
AM_PROG_CC_C_O
LT_AC_PROG_RC
AC_LIBTOOL_WIN32_DLL
AC_LIBTOOL_RC
AC_PROG_LIBTOOL
# test for zlib
zlib=0
AC_CHECK_LIB(z, inflate,zlib=1,zlib=0)
AM_CONDITIONAL(HAVE_ZLIB, test x$zlib = x1)
if test "x$zlib" != "x1"
then
 AC_MSG_ERROR([GMPC plugin requires zlib])
fi



PKG_PROG_PKG_CONFIG
# libxspf 
libxspf=0;
libspiff=0;

AC_ARG_ENABLE([libxspf],
              [--enable-libxspf Use libxspf.], 
	          [case "${enableval}" in
		            yes)    enable_libxspf=true;;
		             no)    enable_libxspf=false;;
		              *)    AC_MSG_ERROR([bad value ${enableval} for --enable-libxspf]);;
               esac],
               [enable_libxspf=detect])
if test "x$enable_libxspf" = "xdetect"; then
	PKG_CHECK_MODULES([libxspf], xspf, enable_libxspf=true, enable_libxspf=false)
fi
if test "x$enable_libxspf" = "xtrue"; then
	PKG_CHECK_MODULES([libxspf], xspf)
	AC_SUBST(libxspf_LIBS)
	AC_SUBST(libxspf_CFLAGS)
    libxspf=1;
	AC_DEFINE([XSPF], 1, [Use xspf])
else

AC_ARG_ENABLE(libspiff,
    [   --enable-libspiff      Use libspiff to parse spiff playlists.],
    [ case "${enableval}" in
        yes)    enable_libspiff=yes;;
         no)    enable_libspiff=no;;
          *)    AC_MSG_ERROR([bad value ${enableval} for --enable-libspiff]);;
     esac],
     [enable_libspiff=auto])

if test "x${enable_libspiff}" != xno; then 
    AC_CHECK_HEADER(spiff/spiff_c.h, AC_CHECK_LIB(spiff, spiff_parse,libspiff=1,libspiff=0),libspiff=0)
    if test "x${libspiff}" = "x1";
    then
        AC_DEFINE(SPIFF,1,["Use or don't use libspiff"])
        SPIFF_LIBS=-lspiff
        AC_SUBST(SPIFF_LIBS)
    else 
        if test "x${enable_libspiff}" = "xyes"; then
            AC_MSG_ERROR([libspiff not found])
        fi
    fi
fi
fi

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
AC_ARG_ENABLE([test],
	[--enable-test Enable tests],
	[case "${enableval}" in
		yes) test_mode=true;;
		no)  test_mode=false;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-test]);;
	 esac],
	[test_mode=false])

AM_CONDITIONAL([ENABLE_TEST], [test x$test_mode = xtrue])

# EXTRA VERSION
AC_ARG_WITH([extra-version],
        [--with-extra-version=revision Specify extra version.],
        with_extra_version=${withval})
if test x"${with_extra_version}" != x; then
    AM_CONDITIONAL([EXTRA_VERSION], [true])
    EXTRA_VERSION="${with_extra_version}";
    AC_SUBST(EXTRA_VERSION)
else
    AM_CONDITIONAL([EXTRA_VERSION], [false])
fi



# Split this out, because pkgconfig macro doesn't return nicely what is missing

# glib
PKG_CHECK_MODULES([glib], glib-2.0 >= 2.16)
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
PKG_CHECK_MODULES([libmpd], libmpd >= 0.19.2)
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

#libsoup
PKG_CHECK_MODULES([libsoup], libsoup-2.4)
AC_SUBST(libsoup_LIBS)
AC_SUBST(libsoup_CFLAGS)

#libgio
PKG_CHECK_MODULES([libgio], gio-2.0)
AC_SUBST(libgio_LIBS)
AC_SUBST(libgio_CFLAGS)


#sqlite3
PKG_CHECK_MODULES([sqlite3], sqlite3)
AC_SUBST(sqlite3_LIBS)
AC_SUBST(sqlite3_CFLAGS)

# libsexy 
AC_ARG_ENABLE([system-libsexy],
              [--enable-system-libsexy Use system libsexy instead of bundles.], 
	          [case "${enableval}" in
		            yes)    system_libsexy=true;;
		             no)    system_libsexy=false;;
		              *)    AC_MSG_ERROR([bad value ${enableval} for --enable-system-libsexy]);;
               esac],
               [system_libsexy=detect])
if test "x$system_libsexy" = "xdetect"; then
	PKG_CHECK_MODULES([libsexy], libsexy, system_libsexy=true, system_libsexy=false)
fi
if test "x$system_libsexy" = "xtrue"; then
	PKG_CHECK_MODULES([libsexy], libsexy)
	AC_SUBST(libsexy_LIBS)
	AC_SUBST(libsexy_CFLAGS)
	AC_DEFINE([USE_SYSTEM_LIBSEXY], 1, [Use system libsexy instead of bundled])
fi

AM_CONDITIONAL(USE_SYSTEM_LIBSEXY, [test x$system_libsexy = xtrue])

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
ALL_LINGUAS="ar bg bn bs cs da de el en_CA en_GB es fi fr gl he hi hu hy id it ja jv lv ml ms nb ne nl pl pt pt_BR ro ru sq sv th tr zh_CN zh_TW"


AM_GLIB_GNU_GETTEXT
GETTEXT_PACKAGE=gmpc
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",[GMPC gettext package])

if test x$gt_cv_have_gettext != "xyes"; then

	echo "Translations support is required."
	exit 1;
fi

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
macige=no
case $host in
  *-*-mingw32* | *-*-windows)
    win32=yes
    EXTRA_CFLAGS="$EXTRA_CFLAGS -mno-cygwin -mwindows -mms-bitfields"
    EXTRA_LDFLAGS="-export-all-symbols -mwindows -mms-bitfields" ;;
  *-*-cygwin*)
    win32=yes
    EXTRA_LDFLAGS="-export-all-symbols -mms-bitfields" ;;
  *-apple-darwin*)
    EXTRA_LDFLAGS="-framework Carbon" 
    macosx=yes;;
esac
AM_CONDITIONAL(WIN32, test x$win32 = xyes)
AM_CONDITIONAL(OSX, test x$macosx = xyes)
# for smclient 
AM_CONDITIONAL(PLATFORM_WIN32, test x$win32 = xyes) 
AM_CONDITIONAL(PLATFORM_OSX, test x$macosx = xyes) 


AC_ARG_ENABLE(macige,
              [  --enable-macige      Enable Mac integration on mac.],
              [ case "${enableval}" in
                    yes)    macige=yes;;
                     no)    macige=no;;
                      *)    AC_MSG_ERROR([bad value ${enableval} for --enable-macige]);;
                esac],
              [macige=no])

if test x$macosx = xyes && test x$macige = xyes; then
# mac integration 
PKG_CHECK_MODULES([macint],ige-mac-integration)
AC_SUBST(macint_CFLAGS)
AC_SUBST(macint_LIBS)
AC_DEFINE(ENABLE_IGE, 1, [Enable Mac integration Support])
fi

##
# These depend on if we are or are not on windows or osX

##
# Session support             
if test x$macosx = xno && test x$win32 = xno ; then
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
else
# force them off
enable_mmkeys=no;
fi
AC_SUBST(EXTRA_CFLAGS)
AC_SUBST(EXTRA_LDFLAGS)

# Check for gob2
GOB2_CHECK(2.0.0)
# Make the gob2 check fatal
if test x${GOB2} = x""; then
	exit 0;
fi

AM_PROG_VALAC([0.7.10])

if test x${VALAC} = x""; then
    exit 0;
fi


AC_ARG_ENABLE([shave],
            [  --disable-shave			Use shave output cleaner when building.],
              [ case "${enableval}" in
                    yes)    enable_shave=yes;;
                     no)    enable_shave=no;;
                      *)    AC_MSG_ERROR([bad value ${enableval} for --disable-shave]);;
                esac],
              [enable_shave=no])
if test x$enable_shave = xyes; then
SHAVE_INIT([./]) 
fi

AC_OUTPUT([
shave
shave-libtool
Makefile
src/Makefile
src/gmpc-version.h
glade/Makefile
data/Makefile
po/Makefile.in
po/Makefile
data/gmpc.pc
remote/Makefile
doc/Makefile
pixmaps/Makefile
pixmaps/icons/Makefile
pixmaps/icons/hicolor/Makefile
pixmaps/icons/hicolor/16x16/Makefile
pixmaps/icons/hicolor/16x16/actions/Makefile
pixmaps/icons/hicolor/16x16/status/Makefile
pixmaps/icons/hicolor/16x16/apps/Makefile
pixmaps/icons/hicolor/22x22/Makefile
pixmaps/icons/hicolor/22x22/actions/Makefile
pixmaps/icons/hicolor/22x22/status/Makefile
pixmaps/icons/hicolor/22x22/apps/Makefile
pixmaps/icons/hicolor/32x32/Makefile
pixmaps/icons/hicolor/32x32/actions/Makefile
pixmaps/icons/hicolor/32x32/status/Makefile
pixmaps/icons/hicolor/32x32/apps/Makefile
pixmaps/icons/hicolor/48x48/Makefile
pixmaps/icons/hicolor/48x48/actions/Makefile
pixmaps/icons/hicolor/48x48/apps/Makefile
pixmaps/icons/hicolor/48x48/status/Makefile
pixmaps/icons/hicolor/64x64/Makefile
pixmaps/icons/hicolor/64x64/apps/Makefile
pixmaps/icons/hicolor/64x64/status/Makefile
pixmaps/icons/hicolor/72x72/Makefile
pixmaps/icons/hicolor/72x72/apps/Makefile
pixmaps/icons/hicolor/72x72/status/Makefile
pixmaps/icons/hicolor/96x96/Makefile
pixmaps/icons/hicolor/96x96/apps/Makefile
pixmaps/icons/hicolor/96x96/status/Makefile
pixmaps/icons/hicolor/128x128/Makefile
pixmaps/icons/hicolor/128x128/apps/Makefile
pixmaps/icons/hicolor/128x128/status/Makefile
pixmaps/icons/hicolor/128x128/categories/Makefile
pixmaps/icons/hicolor/scalable/Makefile
pixmaps/icons/hicolor/scalable/actions/Makefile
pixmaps/icons/hicolor/scalable/status/Makefile
pixmaps/icons/hicolor/scalable/apps/Makefile
pixmaps/icons/hicolor/scalable/categories/Makefile
pixmaps/icons/Humanity/16x16/Makefile
pixmaps/icons/Humanity/16x16/actions/Makefile
pixmaps/icons/Humanity/16x16/status/Makefile
pixmaps/icons/Humanity/16x16/apps/Makefile
pixmaps/icons/Humanity/22x22/Makefile
pixmaps/icons/Humanity/22x22/actions/Makefile
pixmaps/icons/Humanity/22x22/status/Makefile
pixmaps/icons/Humanity/22x22/apps/Makefile
pixmaps/icons/Humanity/24x24/Makefile
pixmaps/icons/Humanity/24x24/actions/Makefile
pixmaps/icons/Humanity/24x24/status/Makefile
pixmaps/icons/Humanity/24x24/categories/Makefile
pixmaps/icons/Humanity/24x24/apps/Makefile
pixmaps/icons/Humanity/32x32/Makefile
pixmaps/icons/Humanity/32x32/actions/Makefile
pixmaps/icons/Humanity/32x32/status/Makefile
pixmaps/icons/Humanity/32x32/apps/Makefile
pixmaps/icons/Humanity/48x48/Makefile
pixmaps/icons/Humanity/48x48/actions/Makefile
pixmaps/icons/Humanity/48x48/apps/Makefile
pixmaps/icons/Humanity/48x48/status/Makefile
pixmaps/icons/Humanity/64x64/Makefile
pixmaps/icons/Humanity/64x64/apps/Makefile
pixmaps/icons/Humanity/64x64/status/Makefile
pixmaps/icons/Humanity/64x64/actions/Makefile
pixmaps/icons/Humanity/72x72/Makefile
pixmaps/icons/Humanity/72x72/apps/Makefile
pixmaps/icons/Humanity/72x72/status/Makefile
pixmaps/icons/Humanity/72x72/actions/Makefile
pixmaps/icons/Humanity/96x96/Makefile
pixmaps/icons/Humanity/96x96/apps/Makefile
pixmaps/icons/Humanity/96x96/status/Makefile
pixmaps/icons/Humanity/96x96/actions/Makefile
pixmaps/icons/Humanity/128x128/Makefile
pixmaps/icons/Humanity/128x128/apps/Makefile
pixmaps/icons/Humanity/128x128/status/Makefile
pixmaps/icons/Humanity/128x128/categories/Makefile
pixmaps/icons/Humanity/128x128/actions/Makefile
pixmaps/icons/Humanity/Makefile
pixmaps/icons/Humanity/scalable/Makefile
pixmaps/icons/Humanity/scalable/actions/Makefile
pixmaps/icons/Humanity/scalable/status/Makefile
pixmaps/icons/Humanity/scalable/apps/Makefile
pixmaps/icons/Humanity/scalable/categories/Makefile
test/Makefile
test/config/Makefile
test/MpdDataModel/Makefile
test/GmpcEasyDownload/Makefile
test/MetaDataCache/Makefile
test/PixbufCache/Makefile
])
echo ""
echo ""
echo "------------------ Status ------------------"
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
if test "x$system_libsexy" = "xtrue"; then 
	echo "Use system libsexy:               enabled"
else
	echo "Use system libsexy:               disabled"
fi
if test "x$macige" = "xyes"; then 
	echo "Use mac integration library:      enabled"
else
	echo "Use mac integration library:      disabled"
fi
if test "x$libspiff" = "x1"; then
	echo "Use libspiff library:             enabled"
else
	echo "Use libspiff library:             disabled"
fi
if test "x$libxspf" = "x1"; then
	echo "Use libxspf library:              enabled"
else
	echo "Use libxspf library:              disabled"
fi
echo "";
echo "Now type make to build"
if test x$enable_shave = xyes; then
    echo "Building is done with reduced output. (shave";
    echo "use --disable-shave to get all (old) output back";
fi
