#!/usr/bin/perl -w
require 5.8;
use bytes;
use strict;

my @msgs;
my $exitcode = 0;
while (<>) {
   while (m!/\*(.*?)\*/([0-9]+)\b!g) {
      if (defined $msgs[$2] && $msgs[$2] ne $1) {
	 print STDERR "Message $2 has two different versions:\n";
	 print STDERR "\"$msgs[$2]\"\n\"$1\"\n";
	 $exitcode = 1;
      }
      $msgs[$2] = $1;
   }
}
my $i = 0;
my $xxx;
for my $msg (@msgs) {
   if (!defined $msg) {
      if (!defined $xxx) {
	 $xxx = $i;
      }
   } else {
      if (defined $xxx) {
	 print "# XXX $xxx";
	 print "-", ($i - 1) if ($i - 1 > $xxx);
	 print "\n";
	 $xxx = undef;
      }
      printf "en:%3d ", $i;
      if ($msg =~ /^\s/ || $msg =~ /\s$/) {
	 print "\"$msg\"\n";
      } else {
	 print "$msg\n";
      }
   }
   ++$i;
}
if (defined $xxx) {
   print "# XXX $xxx";
   print "-", ($i - 1) if ($i - 1 > $xxx);
   print "\n";
   $xxx = undef;
}
exit $exitcode;
