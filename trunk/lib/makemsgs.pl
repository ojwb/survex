#!/usr/bin/perl -w
require 5.008;
use bytes;
use strict;

use integer;

# messages >= this value are written to a header file
my $dontextract_threshold = 1000;

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

open ENT, ($ENV{srcdir}?"$ENV{srcdir}/":"")."named-entities.txt" or die $!;
while (<ENT>) {
   my ($e, $v) = /^(\w+),(\d+)/;
   $ent{$e} = $v;
}
close ENT;

my %msgs = ();
my %dontextract = ();

my %raw = ();
my $raw = '';
my $raw_comments = '';
my $curmsg = -1;

while (<>) {
   if (/^\s*#/) {
       $raw_comments .= $_;
       next; # skip comments
   }

   # en:  0 0.81 the message
   # en-us: 0 0.81 " the message "
   my ($langs, $msgno, $dummy, $msg) = /^([-\w,]+):\s*(\d+)\s+("?)(.*)\3/;

   if ($msgno != $curmsg) {
       if ($msgno < $curmsg) {
	   print STDERR "Warning: message number jumps back from $curmsg to $msgno\n";
       }
       $raw{$curmsg} = $raw;
       $raw = '';
       $curmsg = $msgno;
   }	

   $raw .= $raw_comments . $_;
   $raw_comments = '';

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
      if ($msgno >= $dontextract_threshold) {
	 if (${$dontextract{$_}}[$msgno - $dontextract_threshold]) {
	     print STDERR "Warning: already had message $msgno for language $_\n";
	 }
	 ${$dontextract{$_}}[$msgno - $dontextract_threshold] = $utf8;
      } else {
	 if (${$msgs{$_}}[$msgno]) {
	     print STDERR "Warning: already had message $msgno for language $_\n";
	 }
	 ${$msgs{$_}}[$msgno] = $utf8;
      }
   }
}
$raw{$curmsg} = $raw;

my $lang;
my @langs = sort grep ! /_\*$/, keys %msgs;

my $num_msgs = -1;
foreach $lang (@langs) {
   my $aref = $msgs{$lang};
   $num_msgs = scalar @$aref if scalar @$aref > $num_msgs;
   unlink "$lang.todo";
}

foreach $lang (@langs) {
   my $fnm = $lang;
   $fnm =~ s/(_.*)$/\U$1/;
   open OUT, ">$fnm.msg" or die $!;
   
   my $aref = $msgs{$lang};
 
   my $parentaref;
   my $mainlang = $lang;
   $parentaref = $msgs{$mainlang} if $mainlang =~ s/_.*$//;

   print OUT $magic or die $!;
   print OUT pack("CCn", $major, $minor, $num_msgs) or die $!;

   my $buff = '';
   my $missing = 0;
   my $retranslate = 0;

   for my $n (0 .. $num_msgs - 1) {
      my $warned = 0;
      my $msg = $$aref[$n];
      if (!defined $msg) {
         $msg = $$parentaref[$n] if defined $parentaref;
	 if (!defined $msg) {
	    $msg = ${$msgs{'en'}}[$n];
	    # don't report if we have a parent (as the omission will be
	    # reported there)
	    if (defined $msg && $msg ne '' && !defined $parentaref) {
	       $missing++;
	       open TODO, ">>$lang.todo" or die $!;
	       print TODO $raw{$n}, "\n";
	       close TODO;
	       $warned = 1;
	    } else {
	       $msg = '' if !defined $msg;
	    }
	 }
      } else {
	 if ($lang ne 'en') {
	     sanity_check("Message $n in language $lang", $msg, ${$msgs{'en'}}[$n]);
	 }
      }
      if (defined($raw{$n})
          && $raw{$n} =~ /^\s*#\s*TRANSLATE\b[- \ta-z]*\b$lang\b([^-]|$)/m) {
	 if ($warned) {
	    print STDERR "Warning: message $n missing and also marked for retranslation for language $lang\n";
	 } else {
	    ++$retranslate;
	    open TODO, ">>$lang.todo" or die $!;
	    print TODO $raw{$n}, "\n";
	    close TODO;
	 }
      }
      $buff .= $msg . "\0";
   }

   print OUT pack('N',length($buff)), $buff or die $!;
   close OUT or die $!;

   if ($missing || $retranslate) {
       print STDERR "Warning: see $lang.todo for ";
       if ($missing) {
	   print STDERR "$missing missing message(s)";
	   if ($retranslate) {
	       print STDERR " and $retranslate requiring retranslation";
	   }
       } else {
	   print STDERR "$retranslate message(s) requiring retranslation";
       }
       print STDERR "\n";
   }
}

my $num_dontextract = -1;
foreach $lang (@langs) {
   my $aref = $dontextract{$lang};
   if (defined(@$aref)) {
       $num_dontextract = scalar @$aref if scalar @$aref > $num_dontextract;
   }
}

foreach $lang (@langs) {
   my $fnm = $lang;
   $fnm =~ s/(_.*)$/\U$1/;
   open OUT, ">$fnm.h" or die $!;
   print OUT "#define N_DONTEXTRACTMSGS ", $num_dontextract, "\n";
   print OUT "static unsigned char dontextractmsgs[] =";

   my $missing = 0;
   for my $n (0 .. $num_dontextract - 1) {
      print OUT "\n";

      my $aref = $dontextract{$lang};
 
      my $parentaref;
      my $mainlang = $lang;
      $parentaref = $dontextract{$mainlang} if $mainlang =~ s/_.*$//;

      my $msg = $$aref[$n];
      if (!defined $msg) {
	 $msg = $$parentaref[$n] if defined $parentaref;
	 if (!defined $msg) {
	    $msg = ${$dontextract{'en'}}[$n];
	    # don't report if we have a parent (as the omission will be reported there)
	    if (defined $msg && $msg ne '' && !defined $parentaref) {
	       ++$missing;
	       open TODO, ">>$lang.todo" or die $!;
	       print TODO $raw{$dontextract_threshold + $n}, "\n";
	       close TODO;
	    } else {
	       $msg = '';
	    }
	 }
      } else {
	 if ($lang ne 'en') {
	     sanity_check("Message $n in language $lang", $msg, $$aref[$n]);
	 }
      }
      $msg =~ s/\\/\\\\/g;
      $msg =~ s/"/\\"/g;
      $msg =~ s/\t/\\t/g;
      $msg =~ s/\n/\\n/g;
      $msg =~ s/\r/\\r/g;
      if ($msg =~ /^ / || $msg =~ / $/) {
	 $msg =~ s/\\"/\\\\\\"/g;
	 $msg = '\\"'.$msg.'\\"';
      }
      print OUT "   /*", $dontextract_threshold + $n, "*/ \"$msg\\0\"";
   }
   print OUT ";\n";
   close OUT or die $!;

   if ($missing) {
       print STDERR "Warning: see $lang.todo for $missing missing \"don't extract\" message(s)\n";
   }
}

#for (sort {$a<=>$b} keys %raw) {print "$_ = [$raw{$_}]\n";}

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

sub sanity_check {
   my ($where, $msg, $orig) = @_;
   # check printf-like specifiers match
   # allow valid printf specifiers, or %<any letter> to support strftime
   # and other printf-like formats.
   my @pcent_m = grep /\%/, split /(%(?:[-#0 +'I]*(?:[0-9]*|\*|\*m\$)(?:\.[0-9]*)?(?:hh|ll|[hlLqjzt])?[diouxXeEfFgGaAcsCSpn]|[a-zA-Z]))/, $msg;
   my @pcent_o = grep /\%/, split /(%(?:[-#0 +'I]*(?:[0-9]*|\*|\*m\$)(?:\.[0-9]*)?(?:hh|ll|[hlLqjzt])?[diouxXeEfFgGaAcsCSpn]|[a-zA-Z]))/, $orig;
   while (scalar @pcent_m || scalar @pcent_o) {
       if (!scalar @pcent_m) {
	   print STDERR "Warning: $where misses out \%spec $pcent_o[0]\n";
       } elsif (!scalar @pcent_o) {
	   print STDERR "Warning: $where has extra \%spec $pcent_m[0]\n";
       } elsif ($pcent_m[0] ne $pcent_o[0]) {
	   print STDERR "Warning: $where has \%spec $pcent_m[0] instead of $pcent_o[0]\n";
       }
       pop @pcent_m;
       pop @pcent_o;
   }

   # Check for missing (or added) elipses (...)
   if ($msg =~ /\.\.\./ && $orig !~ /\.\.\./) {
       print STDERR "Warning: $where has elipses but original doesn't\n";
   } elsif ($msg !~ /\.\.\./ && $orig =~ /\.\.\./) {
       print STDERR "Warning: $where is missing elipses\n";
   }

   # Check for missing (or added) menu shortcut (@)
   if ($msg =~ /\@[A-Za-z]/ && $orig !~ /\@[A-Za-z]/) {
       print STDERR "Warning: $where has menu shortcut but original doesn't\n";
   } elsif ($msg !~ /\@[A-Za-z]/ && $orig =~ /\@[A-Za-z]/) {
       print STDERR "Warning: $where is missing menu shortcut\n";
   }

   # Check for missing (or added) menu accelerator "##"
   if ($msg =~ /\#\#/ && $orig !~ /\#\#/) {
       print STDERR "Warning: $where has menu accelerator but original doesn't\n";
   } elsif ($msg !~ /\#\#/ && $orig =~ /\#\#/) {
       print STDERR "Warning: $where is missing menu accelerator\n";
   } elsif ($orig =~ /\#\#(.*)/) {
       my $acc_o = $1;
       my ($acc_m) = $msg =~ /\#\#(.*)/;
       if ($acc_o ne $acc_m) {
	   print STDERR "Warning: $where has menu accelerator $acc_m instead of $acc_o\n";
       }
   }
}
