#!/usr/bin/perl -w
use strict;

print "Dir <Obey\$Dir>\n";

# directories c, h, s, and o are created when the zip file is unpacked

for (@ARGV) {
   s!.*/!!;
   my $f = $_;
   if (s!^(.*)\.([chs])$!$2.$1!) {
      $f =~ s!\.!/!;
      print "Rename $f $_\n";
   }
}

print "Back\n";
