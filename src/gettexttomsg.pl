#!/usr/bin/perl -w
require 5.004;
use strict;

use integer;

# Magic identifier (12 bytes)
my $magic = "Svx\nMsg\r\n\xfe\xff\0";
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

open ENT, "../lib/named-entities.txt" or die $!;
while (<ENT>) {
   my ($e, $v) = /^(\w+),(\d+)/;
   $ent{$e} = $v;
}
close ENT;

my %revmsgs = ();

open MSG, "../lib/messages.txt" or die $!;
while (<MSG>) {
   next if /^\s*#/; # skip comments
   
   # en:  0 0.81 the message
   # en-us: 0 0.81 " the message "
   my ($langs, $msgno, $dummy, $msg) = /^([-\w,]+):\s*(\d+)\s+("?)(.*)\3/;

   unless (defined $langs) {
      chomp;
      print STDERR "Warning: Bad line: \"$_\"\n";
      next;
   }

   $langs =~ tr/-/_/;

   if ($msg =~ /[\0-\x1f\x7f-\xff]/) {
      print STDERR "Warning: literal character in message $msgno\n";
   }

   my $utf8 = string_to_utf8($msg);
   for (split /,/, $langs) {
      if (lc $_ eq 'en') {
	  $revmsgs{$utf8} = $msgno;
      }
   }
}

my $die = 0;

while (<>) {
    if (!/^\s*#/) {
	while (/\\\n$/) {
	    $_ .= <>;
	}
	# very crude - doesn't know about comments, etc
	s!\b_\("(.*?)"\)!"msg(/*$1*/".findmsg($1).")"!gse;
    } elsif (/\s*#\s*define\s+_\(/) {
	$_ = "#include \"message.h\"\n";
    }
    print;
}

if ($die) {
    die "Not all messages found!\n";
}

sub findmsg {
    my $msg = shift;
    $msg =~ s/\\\n//g;
    $msg =~ s/\\n/\n/g;
    $msg =~ s/\\t/\t/g;
    unless (exists $revmsgs{$msg}) {
	if (!$die) {
	    print STDERR "Message(s) not found in message file:\n";
	    $die = 1;
	}
	print STDERR "'$msg'\n";
	return "";
    }
    return $revmsgs{$msg};
}

sub string_to_utf8 {
   my $s = shift;
   $s =~ s/([\x80-\xff])/char_to_utf8(ord($1))/eg;
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
