#!/bin/sh

for file in oneleg midpoint noose cross ; do
  echo $file
  rm -f ./tmp.*
  SURVEXHOME=$srcdir/../lib ../src/cavern $srcdir/$file.svx --output=./tmp > /dev/null
  SURVEXHOME=$srcdir/../lib ../src/diffpos ./tmp.pos $srcdir/$file.pos 0 > /dev/null || exit 1
  rm -f ./tmp.*
done
exit 0
