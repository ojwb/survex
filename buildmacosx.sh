#!/bin/sh
#
# Note: this script requires MacOS X 10.2 or greater, and builds diskimages
# which require MacOS X 10.1 or greater to install.
#
# Currently (at least if built on 10.6) at least 10.6 is required to run.
#
# You probably need to have Xcode installed - you can download this for free
# from Apple: http://developer.apple.com/xcode/
#
# Run from the unpacked survex-1.2.X directory like so:
#
#   ./buildmacosx.sh
#
# This will automatically download and temporarily install wxWidgets
# (this script is smart enough not to download or build it if it already
# has).
#
# If you already have wxWidgets installed permanently, use:
#
#   ./buildmacosx.sh --no-install-wx
#
# If wxWidgets is installed somewhere such that wx-config isn't on your
# PATH you need to indicate where wx-config is by running this script
# something like this:
#
#   env WX_CONFIG=/path/to/wx-config ./buildmacosx.sh
#
# (If you set WX_CONFIG, there's no need to pass --no-install-wx).
#
# If using a pre-installed wxWidgets, note that it must satisfy the
# following requirements:
#   - It must be built with OpenGL support (--with-opengl).
#   - If you build with wx < 3, it probably should be a "Unicode" build
#     (--enable-unicode); wx >= 3 dropped support for non-Unicode builds.

set -e

WX_VERSION=3.0.2
WX_SHA256=346879dc554f3ab8d6da2704f651ecb504a22e9d31c17ef5449b129ed711585d

PROJ_VERSION=4.8.0
PROJ_SHA256=2db2dbf0fece8d9880679154e0d6d1ce7c694dd8e08b4d091028093d87a9d1b5

# Sadly, you can only specify one arch via -arch at a time (a restriction of
# the wxWidgets build system).
#
# Using -arch x86_64 produces a build which will only work on 64-bit Intel
# Macs, but that's probably all machines modern enough to worry about.
# If you want a build which also works on 32 bit Intel Macs, then use
# arch_flags='-arch i386' instead.
#
# To build for much older machines with a ppc CPU, you want arch_flags='-arch
# ppc' instead.
arch_flags='-arch x86_64'
if [ -z "${WX_CONFIG+set}" ] && [ "x$1" != "x--no-install-wx" ] ; then
  if test -x WXINSTALL/bin/wx-config ; then
    :
  else
    prefix=`pwd`/WXINSTALL
    wxtarball=wxWidgets-$WX_VERSION.tar.bz2
    test -f "$wxtarball" || \
      curl -L -O "http://downloads.sourceforge.net/project/wxwindows/$WX_VERSION/$wxtarball"
    if echo "$WX_SHA256  $wxtarball" | shasum -a256 -c ; then
      : # OK
    else
      echo "Checksum of downloaded file '$wxtarball' is incorrect, aborting."
      exit 1
    fi
    echo "+++ Extracting $wxtarball"
    test -d "wxWidgets-$WX_VERSION" || tar jxf "$wxtarball"
    test -d "wxWidgets-$WX_VERSION/build" || mkdir "wxWidgets-$WX_VERSION/build"
    cd "wxWidgets-$WX_VERSION/build"
    # Compliation of wx 3.0.2 fails on OS X 10.10.1 with webview enabled.
    # A build with liblzma enabled doesn't work on OS X 10.6.8.
    ../configure --disable-shared --prefix="$prefix" \
	--with-opengl --enable-unicode \
	--disable-webview --without-liblzma \
	CC="gcc $arch_flags" CXX="g++ $arch_flags"
    make -s
    make -s install
    cd ../..
  fi
  WX_CONFIG=`pwd`/WXINSTALL/bin/wx-config
fi

CC=`$WX_CONFIG --cc`
CXX=`$WX_CONFIG --cxx`

if [ "x$1" != "x--no-install-proj" ] ; then
  if test -f PROJINSTALL/include/proj_api.h ; then
    :
  else
    prefix=`pwd`/PROJINSTALL
    projtarball=proj-$PROJ_VERSION.tar.gz
    test -f "$projtarball" || \
      curl -O "http://download.osgeo.org/proj/$projtarball"
    if echo "$PROJ_SHA256  $projtarball" | shasum -a256 -c ; then
      : # OK
    else
      echo "Checksum of downloaded file '$projtarball' is incorrect, aborting."
      exit 1
    fi
    echo "+++ Extracting $projtarball"
    test -d "proj-$PROJ_VERSION" || tar jxf "$projtarball"
    test -d "proj-$PROJ_VERSION/build" || mkdir "proj-$PROJ_VERSION/build"
    cd "proj-$PROJ_VERSION/build"
    ../configure --disable-shared --prefix="$prefix" CC="$CC" CXX="$CXX"
    make -s
    make -s install
    cd ../..
  fi
fi

test -n "$WX_CONFIG" || WX_CONFIG=`which wx-config`
if test -z "$WX_CONFIG" ; then
  echo "WX_CONFIG not set and wx-config not on your PATH"
  exit 1
fi
# Force static linking so the user doesn't need to install wxWidgets.
WX_CONFIG=$WX_CONFIG' --static'
rm -rf *.dmg Survex macosxtmp
D=`pwd`/Survex
T=`pwd`/macosxtmp
./configure --prefix="$D" --bindir="$D" --mandir="$T" WX_CONFIG="$WX_CONFIG" CC="$CC" CXX="$CXX" CPPFLAGS=-I"`pwd`/PROJINSTALL/include" LDFLAGS=-L"`pwd`/PROJINSTALL/lib"
make
make install
#mv Survex/survex Survex/Survex

# Construct the Aven application bundle.
mkdir Survex/Aven.app
mkdir Survex/Aven.app/Contents
mkdir Survex/Aven.app/Contents/MacOS
mkdir Survex/Aven.app/Contents/Resources
cp lib/Info.plist Survex/Aven.app/Contents
printf APPLAVEN > Survex/Aven.app/Contents/PkgInfo
cp -r "$D"/share/survex/* Survex/Aven.app/Contents/Resources/

# Create Aven.icns containing the icon for Aven.app.
unzip -d Survex/Aven.app/Resources lib/icons/Aven.iconset.zip
iconutil --convert icns Survex/Aven.app/Resources/Aven.iconset
rm -rf Survex/Aven.app/Resources/Aven.iconset

mv Survex/aven Survex/Aven.app/Contents/MacOS/
ln Survex/cavern Survex/Aven.app/Contents/MacOS/
rm -f Survex/share/survex/unifont.pixelfont
rm -rf Survex/share/survex/icons

size=`du -s Survex|sed 's/[^0-9].*//'`
# Allow 1000 extra sectors for various overheads (500 wasn't enough).
sectors=`expr 1000 + "$size"`
# Partition needs to be at least 4M and sectors are 512 bytes.
if test "$sectors" -lt 8192 ; then
  sectors=8192
fi
echo "Creating new blank image survex-macosx.dmg of $sectors sectors"
# This creates the diskimage file and initialises it as an HFS+ volume.
hdiutil create -sectors "$sectors" survex-macosx -layout NONE -fs HFS+ -volname Survex

echo "Presenting image to the filesystems for mounting."
# This will mount the image onto the Desktop.
# Get the name of the device it is mounted on and the mount point.

# man hdiutil says:
# "The output of [hdiutil] attach has been stable since OS X 10.0 (though it
# was called hdid(8) then) and is intended to be program-readable.  It consists
# of the /dev node, a tab, a content hint (if applicable), another tab, and a
# mount point (if any filesystems were mounted)."
#
# In reality, it seems there are also some spaces before each tab character.
hdid_output=`hdid survex-macosx.dmg|tail -1`
echo "Last line of hdid output was: $hdid_output"
dev=`echo "$hdid_output"|sed 's!/dev/\([^	 ]*\).*!\1!'`
mount_point=`echo "$hdid_output"|sed 's!.*	!!'`

echo "Device $dev mounted on $mount_point, copying files into image."
ditto -rsrcFork Survex "$mount_point/Survex"
ditto lib/INSTALL.OSX "$mount_point/INSTALL"

echo "Detaching image."
hdiutil detach "$dev"

version=`sed 's/^VERSION *= *//p;d' Makefile`
file=survex-macosx-$version.dmg
echo "Compressing image file survex-macosx.dmg to $file"
# This needs MacOS X 10.1 or above for unpacking - change UDZO to UDCO to allow
# the dmg to be unpacked on 10.0 as well:
hdiutil convert survex-macosx.dmg -format UDZO -o "$file"
rm survex-macosx.dmg

echo "$file created successfully."
