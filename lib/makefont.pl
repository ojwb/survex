#!/usr/bin/perl -w
require 5.004;
use strict;

use integer;

# Magic identifier (12 bytes)
my $magic = "Svx\nFnt\r\n\xfe\xff\0";
# Designed to be corrupted by ASCII ftp, top bit stripping (or
# being used for parity).  Contains a zero byte so more likely
# to be flagged as data (e.g. by perl's "-B" test).

my $major = 0;
my $minor = 8;

# File format (multi-byte numbers in network order (bigendian)):
# 12 bytes: Magic identifier
# 1 byte:   File format major version number (0)
# 1 byte:   File format minor version number (8)
# 2 bytes:  Number of characters (N)
# 4 bytes:  Offset from XXXX to end of file
# XXXX:
# N*:
# 8 byte bitmap of character (starting from 32)

my %bin2oct = qw(
   000 0 001 1 010 2 011 3
   100 4 101 5 110 6 111 7
   00 0 01 1 10 2 11 3
);

my $buf = '';
my $c = 0;
open IN, ($ARGV[0] || 'pfont.txt') or die $!;
while (<IN>) {
   next if /^-+$/;
   chomp();
   s/\r//;

   s/\./0/g;
   s/#/1/g;
   
   $_ = join "", reverse split //;

   $buf .= chr oct join "", map $bin2oct{$_}, /(..)(...)(...)$/;
   $c++;
}
eof IN or die $!;
close IN;

open OUT, ">".($ARGV[1] || 'pfont.bit') or die $!;
print OUT $magic or die $!;
print OUT pack("CCn", $major, $minor, int($c / 8)) or die $!;

print OUT pack('N',$c), $buf or die $!;
close OUT or die $!;
