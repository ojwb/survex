#!/usr/bin/perl -w
require 5.004;
use strict;

use integer;

my $major = 0;
my $minor = 8;

my %ent = ();

open ENT, "named-entities.txt" or die $!;
while (<ENT>) {
   my ($e, $v) = /^(\w+),(\d+)/;
   $ent{$e} = $v;
}
close ENT;

my %msgs = ();

while (<>) {
   next if /^\d*#/;
   
   # en:  0 0.81 the message
   # en-us: 0 0.81 " the message "
   my ($lang, $msgno, $dummy, $msg) = /^([-\w]+):\s*(\d+)\s+("?)(.*)/;

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

   print OUT "SvxMsg".chr($major).chr($minor) or die $!;
   print OUT pack("N", $num_msgs) or die $!;
   my $offset = 0;

   my $n;
   for $n (0 .. $num_msgs - 1) {
      print OUT pack("N", $offset) or die $!;
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
	 $$aref[$n] = $msg; # poke back into array
      }
      $offset += length($msg) + 1;      
   }
   print OUT join "\xff", @$aref;
   close OUT;
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
