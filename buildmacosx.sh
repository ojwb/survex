#!/bin/sh
#
# Note: this script requires MacOS X 10.2 or greater, and builds diskimages
# which require MacOS X 10.1 or greater to install.
#
# Run from the unpacked survex-1.0.X directory like so:
#
#   ./buildmacosx.sh --install-wx
#
# This will automatically download and temporarily install wxWindows
# (this script is smart enough not to download or build it if it already
# has).
#
# If you already have wxWindows installed permanently, use:
#
#   ./buildmacosx.sh
#
# If wxWindows is installed somewhere such that wx-config isn't on your
# PATH you need to indicate where wx-config is by running this script
# something like this:
#
#   env WXCONFIG=/path/to/wx-config ./buildmacosx.sh

set -e

if test "x$1" = "x--install-wx" ; then
  if test -x WXINSTALL/bin/wx-config ; then
    :
  else
    prefix=`pwd`/WXINSTALL
    test -f wxMac-2.4.2.tar.gz || \
      wget ftp://biolpc22.york.ac.uk/pub/2.4.2/wxMac-2.4.2.tar.gz
    test -d wxMac-2.4.2 || tar zxvf wxMac-2.4.2.tar.gz
    test -d wxMac-2.4.2/build || mkdir wxMac-2.4.2/build
    cd wxMac-2.4.2/build
    ../configure --disable-shared --prefix="$prefix"
    make -s
    make -s install
    cd ../..
  fi
  WXCONFIG=`pwd`/WXINSTALL/bin/wx-config
fi

test -n "$WXCONFIG" || WXCONFIG=`which wx-config`
if test -z "$WXCONFIG" ; then
  echo "WXCONFIG not set and wx-config not on your PATH"
  exit 1
fi
# Force static linking so the user doesn't need to install wxWindows.
WXCONFIG=$WXCONFIG' --static'
export WXCONFIG
rm -rf *.dmg Survex macosxtmp
D=`pwd`/Survex
T=`pwd`/macosxtmp
./configure --prefix="$D" --bindir="$D" --mandir="$T" CFLAGS=-DMACOSX_BUNDLE
make
make install
#mv Survex/survex Survex/Survex

size=`du -s Survex|sed 's/[^0-9].*//'`
# Allow 1000 extra sectors for various overheads (500 wasn't enough).
sectors=`expr 1000 + $size`
# Partition needs to be at least 4M and sectors are 512 bytes.
if test $sectors -lt 8192 ; then
  sectors=8192
fi
echo "Creating new blank image survex-macosx.dmg of $sectors sectors"
# This creates the diskimage file and initialises it as an HFS+ volume.
hdiutil create -sectors $sectors survex-macosx -layout NONE -fs HFS+ -volname Survex

echo "Present image to the filesystems for mounting."
# This will mount the image onto the Desktop.
# Get the name of the device we mounted it on...
dev=`hdid survex-macosx.dmg|tail -1|sed 's!/dev/\([!-~]*\).*!\1!;'`
echo "Mounted on device $dev, copying files into image."
ditto -rsrcFork Survex /Volumes/Survex/Survex
ditto lib/INSTALL.OSX /Volumes/Survex/INSTALL
echo "Detaching image."
hdiutil detach $dev

version=`sed 's/.*AM_INIT_AUTOMAKE([^,]*, *\([0-9.]*\).*/\1/p;d' configure.in`
file=survex-macosx-`echo $version`.dmg
echo "Compressing image file survex-macosx.dmg to $file"
# This needs MacOS X 10.1 or above for unpacking - change UDZO to UDCO to allow
# the dmg to be unpacked on 10.0 as well:
hdiutil convert survex-macosx.dmg -format UDZO -o "$file"
rm survex-macosx.dmg

echo "$file created successfully."
