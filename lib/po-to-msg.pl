#!/usr/bin/perl -w
require 5.008;
use bytes;
use strict;
use Locale::PO;

use integer;

# Magic identifier (12 bytes)
my $magic = "Svx\nMsg\r\n\xfe\xff\0";
# Designed to be corrupted by ASCII ftp, top bit stripping (or
# being used for parity).  Contains a zero byte so more likely
# to be flagged as data (e.g. by perl's "-B" test).

my $srcdir = $0;
$srcdir =~ s!/[^/]+$!!;

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

my %msgs = ();
${$msgs{'en'}}[0] = '©';

# my %uses = ();

my $file;

my %n = ();
my %loc = ();
$file = "$srcdir/survex.pot";
my $num_list = Locale::PO->load_file_asarray($file);
foreach my $po_entry (@{$num_list}) {
    my $ref = $po_entry->reference;
    (defined $ref && $ref =~ /^n:(\d+)$/m) or next;
    my $msgno = $1;
    my $key = $po_entry->msgid;
    my $msg = c_unescape($po_entry->dequote($key));
    my $where = $file . ":" . $po_entry->loaded_line_number;
    ${$loc{'en'}}[$msgno] = $where;
    if (${$msgs{'en'}}[$msgno]) {
	print STDERR "$where: warning: already had message $msgno for language 'en'\n";
    }
    ${$msgs{'en'}}[$msgno] = $msg;
    ++$n{$msgno};
}
my $last = 0;
for (sort { $a <=> $b } keys %n) {
    if ($_ > $last + 1) {
	print STDERR "$file: Unused msg numbers: ", join(" ", $last + 1 .. $_ - 1), "\n";
    }
    $last = $_;
}
print STDERR "$file: Last used msg number: $last\n";
%n = ();

my %fuzzy;
my %c_format;
for my $po_file (@ARGV) {
    my $language = $po_file;
    $language =~ s/\.po$//;

    $file = "$srcdir/$po_file";
    my $po_hash = Locale::PO->load_file_ashash($file);

    if (exists $$po_hash{'""'}) {
	if ($$po_hash{'""'}->msgstr =~ /^(?:.*\\n)?Language:\s*([^\s\\]+)/im) {
	    if ($language ne $1) {
		my $line = 3 + scalar(@{[$& =~ /(\\n)/g]});
		print STDERR "$file:$line: Language code '$1' doesn't match '$language' from filename\n";
	    }
	} else {
	    my $line = 2 + scalar(@{[$$po_hash{'""'}->msgstr =~ /(\\n)/g]});
	    print STDERR "$file:$line: No suitable 'Language:' field in header\n";
	}
    } else {
	print STDERR "$file:1: Expected 'msgid \"\"' with header\n";
    }

    my $fuzzy = 0;
    foreach my $po_entry (@{$num_list}) {
	my $ref = $po_entry->reference;
	(defined $ref && $ref =~ /^n:(\d+)$/m) or next;
	my $msgno = $1;
	my $key = $po_entry->msgid;
	my $ent = $$po_hash{$key};
	my $where = $file . ":" . $po_entry->loaded_line_number;
	${$loc{$language}}[$msgno] = $where;
	if (defined $ent) {
	    my $msg = c_unescape($po_entry->dequote($ent->msgstr));
	    if ($msg eq '') {
		print STDERR "$where: warning: Empty translation marked as fuzzy\n" if $ent->fuzzy();
		next;
	    }
	    if (${$msgs{$language}}[$msgno]) {
		print STDERR "$where: warning: already had message $msgno for language $language\n";
	    }
	    ${$msgs{$language}}[$msgno] = $msg;
	    $ent->fuzzy() and ++$fuzzy;
	}
	$po_entry->c_format and $c_format{$language}[$msgno]++;
    }
    $fuzzy{$language} = $fuzzy;
}

my $lang;
my @langs = sort grep ! /_\*$/, keys %msgs;

my $num_msgs = -1;
foreach $lang (@langs) {
   my $aref = $msgs{$lang};
   $num_msgs = scalar @$aref if scalar @$aref > $num_msgs;
}

foreach $lang (@langs) {
   my $fnm = $lang;
   $file = "$srcdir/$lang.po";
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

   for my $n (0 .. $num_msgs - 1) {
      my $warned = 0;
      my $msg = $$aref[$n];
      if (!defined $msg) {
	 $msg = $$parentaref[$n] if defined $parentaref;
	 if (!defined $msg) {
	    $msg = ${$msgs{'en'}}[$n];
	    # don't report if we have a parent (as the omission will be
	    # reported there)
	    if (defined $msg && $msg ne '' && $msg ne '©' && !defined $parentaref) {
	       ++$missing;
	       $warned = 1;
	    } else {
	       $msg = '' if !defined $msg;
	    }
	 }
      } else {
	 if ($lang ne 'en') {
	     my $c_format = $c_format{$lang}[$n] // 0;
	     sanity_check("Message $n in language $lang", $msg, ${$msgs{'en'}}[$n], ${$loc{$lang}}[$n], $c_format);
	 }
      }
      $buff .= $msg . "\0";
   }

   print OUT pack('N',length($buff)), $buff or die $!;
   close OUT or die $!;

   my $fuzzy = $fuzzy{$lang};
   if ($missing || $fuzzy) {
       print STDERR "Warning: $file: ";
       if ($missing) {
	   print STDERR "$missing missing message(s)";
	   if ($fuzzy) {
	       print STDERR " and $fuzzy fuzzy message(s)";
	   }
       } else {
	   print STDERR "$fuzzy fuzzy message(s)";
       }
       print STDERR " for $lang\n";
   }
}

sub sanity_check {
   my ($what, $msg, $orig, $where, $c_format) = @_;
   if ($c_format) {
      # check printf-like specifiers match
      # allow valid printf specifiers, or %<any letter> to support strftime
      # and other printf-like formats.
      my @pcent_m = grep /\%/, split /(%(?:[-#0 +'I]*(?:[0-9]*|\*|\*m\$)(?:\.[0-9]*)?(?:hh|ll|[hlLqjzt])?[diouxXeEfFgGaAcsCSpn]|[a-zA-Z]))/, $msg;
      my @pcent_o = grep /\%/, split /(%(?:[-#0 +'I]*(?:[0-9]*|\*|\*m\$)(?:\.[0-9]*)?(?:hh|ll|[hlLqjzt])?[diouxXeEfFgGaAcsCSpn]|[a-zA-Z]))/, $orig;
      while (scalar @pcent_m || scalar @pcent_o) {
	  if (!scalar @pcent_m) {
	      print STDERR "$where: warning: $what misses out \%spec $pcent_o[0]\n";
	  } elsif (!scalar @pcent_o) {
	      print STDERR "$where: warning: $what has extra \%spec $pcent_m[0]\n";
	  } elsif ($pcent_m[0] ne $pcent_o[0]) {
	      print STDERR "$where: warning: $what has \%spec $pcent_m[0] instead of $pcent_o[0]\n";
	  }
	  pop @pcent_m;
	  pop @pcent_o;
      }
   }

   # Check for missing (or added) ellipses (...)
   if ($msg =~ /\.\.\./ && $orig !~ /\.\.\./) {
       print STDERR "$where: warning: $what has ellipses but original doesn't\n";
   } elsif ($msg !~ /\.\.\./ && $orig =~ /\.\.\./) {
       print STDERR "$where: warning: $what is missing ellipses\n";
   }

   # Check for missing (or added) menu shortcut (&)
   if ($msg =~ /\&[A-Za-z\xc2-\xf4]/ && $orig !~ /\&[A-Za-z]/) {
       print STDERR "$where: warning: $what has menu shortcut but original doesn't\n";
   } elsif ($msg !~ /\&[A-Za-z\xc2-\xf4]/ && $orig =~ /\&[A-Za-z]/) {
       print STDERR "$where: warning: $what is missing menu shortcut\n";
   }

   # Check for missing (or added) double quotes (“ and ”)
   if (scalar($msg =~ s/(?:“|»)/$&/g) != scalar($orig =~ s/“/$&/g)) {
       print STDERR "$where: warning: $what has different numbers of “\n";
       print STDERR "$orig\n$msg\n\n";
   }
   if (scalar($msg =~ s/(?:”|«)/$&/g) != scalar($orig =~ s/”/$&/g)) {
       print STDERR "$where: warning: $what has different numbers of ”\n";
       print STDERR "$orig\n$msg\n\n";
   }

   # Check for missing (or added) menu accelerator "##"
   if ($msg =~ /\#\#/ && $orig !~ /\#\#/) {
       print STDERR "$where: warning: $what has menu accelerator but original doesn't\n";
   } elsif ($msg !~ /\#\#/ && $orig =~ /\#\#/) {
       print STDERR "$where: warning: $what is missing menu accelerator\n";
   } elsif ($orig =~ /\#\#(.*)/) {
       my $acc_o = $1;
       my ($acc_m) = $msg =~ /\#\#(.*)/;
       if ($acc_o ne $acc_m) {
	   print STDERR "$where: warning: $what has menu accelerator $acc_m instead of $acc_o\n";
       }
   }

   # Check for missing (or added) menu accelerator "\t"
   if ($msg =~ /\t/ && $orig !~ /\t/) {
       print STDERR "$where: warning: $what has menu accelerator but original doesn't\n";
   } elsif ($msg !~ /\t/ && $orig =~ /\t/) {
       print STDERR "$where: warning: $what is missing menu accelerator\n";
   } elsif ($orig =~ /\t(.*)/) {
       my $acc_o = $1;
       my ($acc_m) = $msg =~ /\t(.*)/;
       if ($acc_o ne $acc_m) {
	   print STDERR "$where: warning: $what has menu accelerator $acc_m instead of $acc_o\n";
       }
   }
}

sub c_unescape {
   my $str = shift @_;
   $str =~ s/\\(x..|0|[0-7][0-7][0-7]|.)/&c_unescape_char($1)/ge;
   return $str;
}

sub c_unescape_char {
    my $ch = shift @_;
    if ($ch eq '0' || $ch eq 'x00' || $ch eq '000') {
	print STDERR "Nul byte in translation! (\\$ch)\n";
	exit(1);
    }
    return $ch if $ch eq '"' || $ch eq '\\';
    return "\n" if $ch eq "n";
    return "\t" if $ch eq "t";
    return "\r" if $ch eq "r";
    return "\f" if $ch eq "f";
    return chr(hex(substr($ch,1))) if $ch =~ /^x../;
    return chr(oct($ch)) if $ch =~ /^[0-7][0-7][0-7]/;
    print STDERR "Unknown C-escape in translation! (\\$ch)\n";
    exit(1);
}
