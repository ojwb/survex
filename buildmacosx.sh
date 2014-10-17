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

WXVERSION=3.0.2
WX_SHA256=346879dc554f3ab8d6da2704f651ecb504a22e9d31c17ef5449b129ed711585d

# Sadly, you can only specify one arch via -arch at a time (a restriction of
# the wxWidgets build system).
#
# Using -arch i386 produces a build which will also work on 64-bit Intel Macs.
# If you want a build which *only* works on 64 bit Intel Macs, then use
# arch_flags='-arch x86_64' instead.
#
# To build for older machines with a ppc CPU, you want arch_flags='-arch ppc'
# instead.
arch_flags='-arch i386'
if [ -z "${WX_CONFIG+set}" ] && [ "x$1" != "x--no-install-wx" ] ; then
  if test -x WXINSTALL/bin/wx-config ; then
    :
  else
    prefix=`pwd`/WXINSTALL
    wxtarball=wxWidgets-$WXVERSION.tar.bz2
    test -f "$wxtarball" || \
      curl -O "http://ftp.wxwidgets.org/pub/$WXVERSION/$wxtarball"
    if echo "$WX_SHA256  $wxtarball" | shasum -a256 -c ; then
      : # OK
    else
      echo "Checksum of downloaded file '$wxtarball' is incorrect, aborting."
      exit 1
    fi
    echo "+++ Extracting $wxtarball"
    test -d "wxWidgets-$WXVERSION" || tar jxf "$wxtarball"
    test -d "wxWidgets-$WXVERSION/build" || "mkdir wxWidgets-$WXVERSION/build"
    cd "wxWidgets-$WXVERSION/build"
    ../configure --disable-shared --prefix="$prefix" --with-opengl --enable-unicode CC="gcc $arch_flags" CXX="g++ $arch_flags"
    make -s
    make -s install
    cd ../..
  fi
  WX_CONFIG=`pwd`/WXINSTALL/bin/wx-config
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
./configure --prefix="$D" --bindir="$D" --mandir="$T" WX_CONFIG="$WX_CONFIG" CC="gcc $arch_flags" CXX="g++ $arch_flags"
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
# FIXME: Generate Survex/Aven.app/Resources/Aven.icns
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
mount_point=`echo "$hdid_output"|sed 's!.*[	 ]!!'`

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
