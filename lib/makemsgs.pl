#!/usr/bin/perl -w
require 5.004;
use strict;

use integer;

# Magic identifier (12 bytes)
my $magic = "Svx\n\Msg\r\n\xfe\xff\0";
# Designed to be corrupted by ASCII ftp, top bit stripping (or
# being used for parity).  Contains a zero byte so more likely
# to be flagged as data (e.g. by perl's "-B" test).

my $major = 0;
my $minor = 8;

# File format (multi-byte numbers in network order (bigendian)):
# 12 bytes: Magic identifier
# 1 byte:   File format major version number (0)
# 1 byte:   File format minor version number (8)
# 2 bytes:  Number of messages (N)
# 4 bytes:  Offset from XXXX to end of file
# XXXX:
# N*:
# <message> NUL

my %ent = ();

open ENT, "named-entities.txt" or die $!;
while (<ENT>) {
   my ($e, $v) = /^(\w+),(\d+)/;
   $ent{$e} = $v;
}
close ENT;

my %msgs = ();

while (<>) {
   next if /^\s*#/; # skip comments
   
   # en:  0 0.81 the message
   # en-us: 0 0.81 " the message "
   my ($lang, $msgno, $dummy, $msg) = /^([-\w]+):\s*(\d+)\s+("?)(.*)\3/;

   unless (defined $lang) {
      chomp;
      print STDERR "Warning: Bad line: \"$_\"\n";
      next;
   }

   if ($msg =~ /[\0-\x1f\x7f-\xff]/) {
      print STDERR "Warning: literal character in message $msgno\n";
   }

   ${$msgs{$lang}}[$msgno] = string_to_utf8($msg);
}

my $lang;
my @langs = sort keys %msgs;

my $num_msgs = -1;
foreach $lang (@langs) {
   my $aref = $msgs{$lang};
   $num_msgs = scalar @$aref if scalar @$aref > $num_msgs;
}

foreach $lang (@langs) {
   open OUT, ">$lang.msg" or die $!;
   
   my $aref = $msgs{$lang};
 
   my $parentaref;
   my $mainlang = $lang;
   $parentaref = $msgs{$mainlang} if $mainlang =~ s/-.*$//;

   print OUT $magic or die $!;
   print OUT pack("CCn", $major, $minor, $num_msgs) or die $!;

   my $buff = '';

   my $n;
   for $n (0 .. $num_msgs - 1) {
      my $msg = $$aref[$n];
      if (!defined $msg) {
         $msg = $$parentaref[$n] if defined $parentaref;
	 if (!defined $msg) {
	    $msg = ${$msgs{'en'}}[$n];
	    if (defined $msg && $msg ne '') {
	       print STDERR "Warning: message $n not in language $lang\n";
	    } else {
	       $msg = '';
	    }
	 }
      }
      $buff .= $msg . "\0";
   }
   
   print OUT pack('N',length($buff)), $buff or die $!;
   close OUT or die $!;
}

sub string_to_utf8 {
   my $s = shift;
   $s =~ s/[\x80-\xff]/char_to_utf8(ord$1)/eg;
   $s =~ s/\&(#\d+|#x[a-f0-9]+|[a-z0-9]+);?/decode_entity($1)/eig;
   return $s;
}

sub decode_entity {
   my $ent = shift;
   return char_to_utf8($1) if $ent =~ /^#(\d+)$/;
   return char_to_utf8(hex($1)) if $ent =~ /^#x([a-f0-9]+)$/;
   return char_to_utf8($ent{$ent}) if exists $ent{$ent};
   $ent = "\&$ent;";
   print STDERR "Warning: entity \"$ent\" unknown\n";
   return $ent;
}

sub char_to_utf8 {
   my $unicode = shift;
   # ASCII is easy, and the most common case
   return chr($unicode) if $unicode < 0x80;

   my $result = '';
   my $n = 0x20;
   while (1) {
      $result = chr(0x80 | ($unicode & 0x3f)) . $result;
      $unicode >>= 6;
      last if $unicode < $n;
      $n >>= 1;
   }
   $result = chr((0x100 - $n*2) | $unicode) . $result;
   return $result;
}
