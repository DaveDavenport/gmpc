#!/bin/sh
# Run this to set up the build system: configure, makefiles, etc.
# (based on the version in enlightenment's cvs)

package="mpc"

olddir=`pwd`
srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

cd "$srcdir"
DIE=0

echo "checking for autoconf... "
(autoconf --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have autoconf installed to compile $package."
        echo "Download the appropriate package for your distribution,"
        echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
        DIE=1
}

VERSIONGREP="sed -e s/.*[^0-9\.]\([0-9]\.[0-9]\).*/\1/"
VERSIONMKINT="sed -e s/[^0-9]//"
                                                                                
# do we need automake?
if test -r Makefile.am; then
  AM_NEEDED=`fgrep AUTOMAKE_OPTIONS Makefile.am | $VERSIONGREP`
  if test -z $AM_NEEDED; then
    echo -n "checking for automake... "
    AUTOMAKE=automake
    ACLOCAL=aclocal
    if ($AUTOMAKE --version < /dev/null > /dev/null 2>&1); then
      echo "no"
      AUTOMAKE=
    else
      echo "yes"
    fi
  else
    echo -n "checking for automake $AM_NEEDED or later... "
    for am in automake-$AM_NEEDED automake$AM_NEEDED automake; do
      ($am --version < /dev/null > /dev/null 2>&1) || continue
      ver=`$am --version < /dev/null | head -n 1 | $VERSIONGREP | $VERSIONMKINT`
      verneeded=`echo $AM_NEEDED | $VERSIONMKINT`
      if test $ver -ge $verneeded; then
        AUTOMAKE=$am
        echo $AUTOMAKE
        break
      fi
    done
    test -z $AUTOMAKE &&  echo "no"
    echo -n "checking for aclocal $AM_NEEDED or later... "
    for ac in aclocal-$AM_NEEDED aclocal$AM_NEEDED aclocal; do
      ($ac --version < /dev/null > /dev/null 2>&1) || continue
      ver=`$ac --version < /dev/null | head -n 1 | $VERSIONGREP | $VERSIONMKINT`
      verneeded=`echo $AM_NEEDED | $VERSIONMKINT`
      if test $ver -ge $verneeded; then
        ACLOCAL=$ac
        echo $ACLOCAL
        break
      fi
    done
    test -z $ACLOCAL && echo "no"
  fi
  test -z $AUTOMAKE || test -z $ACLOCAL && {
        echo
        echo "You must have automake installed to compile $package."
        echo "Download the appropriate package for your distribution,"
        echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
        exit 1
  }
fi

#echo -n "checking for libtool... "
#for LIBTOOLIZE in libtoolize glibtoolize nope; do
#  (which $LIBTOOLIZE) > /dev/null 2>&1 && break
#done
#if test x$LIBTOOLIZE = xnope; then
#  echo "nope."
#  LIBTOOLIZE=libtoolize
#else
#  echo $LIBTOOLIZE
#fi
#($LIBTOOLIZE --version) < /dev/null > /dev/null 2>&1 || {
#	echo
#	echo "You must have libtool installed to compile $package."
#	echo "Download the appropriate package for your system,"
#	echo "or get the source from one of the GNU ftp sites"
#	echo "listed in http://www.gnu.org/order/ftp.html"
#	DIE=1
#}

if test "$DIE" -eq 1; then
        exit 1
fi

echo "Generating configuration files for $package, please wait...."

if [ -d /usr/local/share/aclocal ]; then
	ACLOCAL_FLAGS="$ACLOCAL_FLAGS -I /usr/local/share/aclocal"
fi
echo "  $ACLOCAL $ACLOCAL_FLAGS"
$ACLOCAL $ACLOCAL_FLAGS

echo "  autoheader"
autoheader

#echo "  $LIBTOOLIZE --automake"
#$LIBTOOLIZE --automake

echo "  $AUTOMAKE --add-missing $AUTOMAKE_FLAGS"
$AUTOMAKE --add-missing $AUTOMAKE_FLAGS 

echo "  autoconf"
autoconf

cd $olddir
$srcdir/configure "$@" && echo
