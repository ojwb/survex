#!/bin/sh

for file in oneleg midpoint noose cross ; do
  echo $file
  rm -f ./tmp.*
  SURVEXHOME=$srcdir/../lib ../src/cavern $srcdir/$file.svx --output=./tmp > /dev/null
  ../src/diffpos tmp.pos $file.pos 0 > /dev/null || exit 1
  rm -f ./tmp.*
done
exit 0
