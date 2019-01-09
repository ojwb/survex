#!/bin/sh
#
# You'll need some development tools which aren't installed by default.  To
# get these you can install Xcode (which is a free download from Apple):
#
#   https://developer.apple.com/xcode/downloads/
#
# To build, open a terminal (Terminal.app in the "Utils" folder in
# "Applications") and unpack the downloaded survex source code. E.g. for
# 1.2.17:
#
#   tar xf survex-1.2.17.tar.gz
#
# Then change directory into the unpacked sources:
#
#   cd survex-1.2.17
#
# And then run this script:
#
#   ./buildmacosx.sh
#
# This will automatically download and temporarily install wxWidgets,
# PROJ.4 and FFmpeg in subdirectories of the source tree (this script is smart
# enough not to re-download or re-build these if it already has).
#
# If you already have wxWidgets, PROJ.4 and/or FFmpeg installed permanently,
# you can disable these by passing one or more extra options - e.g. to build
# none of them, use:
#
#   ./buildmacosx.sh --no-install-wx --no-install-proj --no-install-ffmpeg
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
#
#   - It must be built with OpenGL support (--with-opengl).
#   - It must be wxWidgets >= 3.0
#
# This script builds diskimages which are known to work at least as far back
# as OS X 10.6.8.  A build of wxWidgets 3.0.2 with the options we pass will
# default to assuming at least OS X 10.5, but we've not heard from anyone
# trying with such an old version.
#
# Mac OS X support "fat" binaries which contain code for more than one
# processor, but the wxWidgets build system doesn't seem to allow creating
# these, so we have to choose what processor family to build for.  By default
# we build for the architecture of the version of OS X the build machine is
# running, which on a modern Mac will be x86_64, producing a build which will
# only work on 64-bit Intel Macs (but that's probably all machines modern
# enough to worry about).
#
# You can specify an arch explicitly on the command-line, e.g. to force a
# build for 64-bit Intel Macs:
#
#   ./buildmacosx.sh x86_64
#
# Or if you want a build which also works on older 32 bit Intel Macs, then run
# this script passing i386 on the command line, like so (assuming your compiler
# supports this):
#
#   ./buildmacosx.sh i386
#
# Or to build for much older machines with a Power PC processor, use (again
# assuming your compiler supports this):
#
#   ./buildmacosx.sh ppc

set -e

install_wx=yes
install_proj=yes
install_ffmpeg=yes
while [ "$#" != 0 ] ; do
  case $1 in
    --no-install-wx)
      install_wx=no
      shift
      ;;
    --no-install-proj)
      install_proj=no
      shift
      ;;
    --no-install-ffmpeg)
      install_ffmpeg=no
      shift
      ;;
    --help|-h)
      echo "Usage: $0 [--no-install-wx] [--no-install-proj] [--no-install-ffmpeg] [ppc|i386|x86_86]"
      exit 0
      ;;
    -*)
      echo "Unknown option - try --help" >&2
      exit 1
      ;;
    *)
      break
      ;;
  esac
done

arch=$1
if [ -z "$arch" ] ; then
  arch=`uname -m`
fi
arch_flags="-arch $arch"

# UDBZ means the resultant disk image will only open on OS X 10.4 or above.
# UDZO works on 10.1 and later, but is larger,  UDCO works on 10.0 as well,
# but is larger still.
dmg_format=UDBZ

WX_VERSION=3.0.4
WX_SHA256=96157f988d261b7368e5340afa1a0cad943768f35929c22841f62c25b17bf7f0

PROJ_VERSION=4.9.3
PROJ_SHA256=6984542fea333488de5c82eea58d699e4aff4b359200a9971537cd7e047185f7

FFMPEG_VERSION=3.1.3
FFMPEG_SHA256=f8575c071e2a64437aeb70c8c030b385cddbe0b5cde20c9b18a6def840128822

if [ -z "${WX_CONFIG+set}" ] && [ "$install_wx" != no ] ; then
  if test -x WXINSTALL/bin/wx-config ; then
    :
  else
    prefix=`pwd`/WXINSTALL
    wxtarball=wxWidgets-$WX_VERSION.tar.bz2
    test -f "$wxtarball" || \
      curl -L -O "https://github.com/wxWidgets/wxWidgets/releases/download/v$WX_VERSION/$wxtarball"
    if echo "$WX_SHA256  $wxtarball" | shasum -a256 -c ; then
      : # OK
    else
      echo "Checksum of downloaded file '$wxtarball' is incorrect, aborting."
      exit 1
    fi
    echo "+++ Extracting $wxtarball"
    test -d "wxWidgets-$WX_VERSION" || tar xf "$wxtarball"
    test -d "wxWidgets-$WX_VERSION/BUILD" || mkdir "wxWidgets-$WX_VERSION/BUILD"
    cd "wxWidgets-$WX_VERSION/BUILD"
    # Compilation of wx 3.0.2 fails on OS X 10.10.1 with webview enabled.
    # A build with liblzma enabled doesn't work on OS X 10.6.8.
    ../configure --disable-shared --prefix="$prefix" \
	--with-opengl --enable-display \
	--disable-webview --without-liblzma \
	CC="gcc $arch_flags" CXX="g++ $arch_flags"
    make -s
    make -s install
    cd ../..
  fi
  WX_CONFIG=`pwd`/WXINSTALL/bin/wx-config
fi

[ -n "${WX_CONFIG}" ] || WX_CONFIG=`which wx-config 2>/dev/null`
if [ -z "${WX_CONFIG}" ] ; then
  echo "wx-config not found in PATH."
  echo
  echo "If wxWidgets is installed, you can specify its location by setting environment variable WX_CONFIG:"
  echo
  echo "WX_CONFIG=/opt/bin/wx-config sh ./buildmacosx.sh"
  exit 1
fi
CC=`$WX_CONFIG --cc`
CXX=`$WX_CONFIG --cxx`

if [ "$install_proj" != no ] ; then
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
    test -d "proj-$PROJ_VERSION" || tar xf "$projtarball"
    test -d "proj-$PROJ_VERSION/BUILD" || mkdir "proj-$PROJ_VERSION/BUILD"
    cd "proj-$PROJ_VERSION/BUILD"
    ../configure --disable-shared --prefix="$prefix" CC="$CC" CXX="$CXX"
    make -s
    make -s install
    cd ../..
  fi
fi

if [ "$install_ffmpeg" != no ] ; then
  if test -f FFMPEGINSTALL/include/libavcodec/avcodec.h ; then
    :
  else
    prefix=`pwd`/FFMPEGINSTALL
    ffmpegtarball=ffmpeg-$FFMPEG_VERSION.tar.xz
    test -f "$ffmpegtarball" || \
      curl -O "https://ffmpeg.org/releases/$ffmpegtarball"
    if echo "$FFMPEG_SHA256  $ffmpegtarball" | shasum -a256 -c ; then
      : # OK
    else
      echo "Checksum of downloaded file '$ffmpegtarball' is incorrect, aborting."
      exit 1
    fi
    echo "+++ Extracting $ffmpegtarball"
    test -d "ffmpeg-$FFMPEG_VERSION" || tar xf "$ffmpegtarball"
    test -d "ffmpeg-$FFMPEG_VERSION/BUILD" || mkdir "ffmpeg-$FFMPEG_VERSION/BUILD"
    cd "ffmpeg-$FFMPEG_VERSION/BUILD"
    FFMPEG_CONFIGURE_OPTS='--disable-shared --disable-programs --disable-network --disable-bsfs --disable-protocols --disable-devices'
    # We don't need to decode video, but disabling these causes a link failure
    # when linking aven:
    # --disable-decoders --disable-demuxers
    if ! nasm -hf|grep -q macho64 ; then
      # nasm needs to support macho64, at least for x86_64 builds.
      FFMPEG_CONFIGURE_OPTS="$FFMPEG_CONFIGURE_OPTS --disable-yasm"
    fi
    ../configure --prefix="$prefix" --cc="$CC" $FFMPEG_CONFIGURE_OPTS
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
C=`pwd`
D=$C/Survex
T=$C/macosxtmp
PKG_CONFIG_PATH=$C/PROJINSTALL/lib/pkgconfig:$C/FFMPEGINSTALL/lib/pkgconfig
export PKG_CONFIG_PATH
./configure --prefix="$D" --bindir="$D" --mandir="$T" \
    WX_CONFIG="$WX_CONFIG" CC="$CC" CXX="$CXX"
make
make install
#mv Survex/survex Survex/Survex

# Construct the Aven application bundle.
make create-aven-app APP_PATH=Survex/Aven.app
mv Survex/share/doc/survex Survex/Docs
rmdir Survex/share/doc
mv Survex/share/survex/images Survex/Aven.app/Contents/Resources/
mv Survex/share/survex/unifont.pixelfont Survex/Aven.app/Contents/Resources/
ln Survex/share/survex/*.msg Survex/Aven.app/Contents/Resources/
mv Survex/aven Survex/Aven.app/Contents/MacOS/
ln Survex/cavern Survex/extend Survex/Aven.app/Contents/MacOS/
rm -rf Survex/share/applications Survex/share/icons Survex/share/mime-info
mkdir Survex/share/survex/proj
cp -p PROJINSTALL/share/proj/epsg PROJINSTALL/share/proj/esri Survex/share/survex/proj
mkdir Survex/Aven.app/Contents/Resources/proj
ln Survex/share/survex/proj/* Survex/Aven.app/Contents/Resources/proj

size=`du -s Survex|sed 's/[^0-9].*//'`
# Allow 1500 extra sectors for various overheads (1000 wasn't enough).
sectors=`expr 1500 + "$size"`
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
hdiutil convert survex-macosx.dmg -format "$dmg_format" -o "$file"
rm survex-macosx.dmg

echo "$file created successfully."
