#!/bin/sh
#
# Run from the unpacked survex-1.0.X directory like so:
#
#   ./buildmacosx.sh
#
# If wxWindows is installed somewhere such that wx-config isn't on your
# PATH you need to indicate where wx-config is by running this script
# something like this:
#
#   env WXCONFIG=/path/to/wx-config ./buildmacosx.sh

set -e
test -n "$WXCONFIG" || WXCONFIG=`which wx-config`
if test -z "$WXCONFIG" ; then
  echo "WXCONFIG not set and wx-config not on your PATH"
  exit 1
fi
# Force static linking so the user doesn't need to install wxWindows.
WXCONFIG=$WXCONFIG' --static'
export WXCONFIG
rm -rf *.dmg Survex macosx
D="`pwd`/Survex"
T="`pwd`/macosxtmp"
./configure --prefix="$D" --bindir="$D" --mandir="$T"
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
# This just writes ASCII data to the file until it's the correct size.
hdiutil create -sectors $sectors survex-macosx -layout NONE

# Get the name of the next available device that can be used for mounting
# (attaching).
dev=`hdid -nomount survex-macosx.dmg|tail -1|sed 's!/dev/!!'`
echo "Constructing new HFS+ filesystem on $dev"
# This will initialize /dev/r$dev as a HFS Plus volume.
sudo newfs_hfs -v Survex /dev/r$dev
# We have to eject (detach) the device.
# Note: 'hdiutil info' will show what devices are still attached.
hdiutil eject $dev

echo "Present image to the filesystems for mounting."
# This will mount the image onto the Desktop.
hdid survex-macosx.dmg
ditto -rsrcFork Survex /Volumes/Survex/Survex
ditto lib/INSTALL.OSX /Volumes/Survex/INSTALL
hdiutil eject $dev

version="`sed 's/.*AM_INIT_AUTOMAKE([^,]*, *\([0-9.]*\).*/\1/p;d' configure.in`"
file="survex-macosx-`echo $version`.dmg"
echo "Compressing image file survex-macosx.dmg to $file"
hdiutil convert survex-macosx.dmg -format UDCO -o "$file"
# Better compression but needs MacOS X 10.1 or above for unpacking:
#hdiutil convert survex-macosx.dmg -format UDZO -o "$file"

echo "$file created successfully."
