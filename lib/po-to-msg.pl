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

my %n = ();
my $num_list = Locale::PO->load_file_asarray("$srcdir/po_codes");
foreach my $po_entry (@{$num_list}) {
    my $msgno = $po_entry->dequote($po_entry->msgstr);
    next if $msgno !~ /^\d+$/;
    my $key = $po_entry->msgid;
    my $msg = c_unescape($po_entry->dequote($key));
    if (${$msgs{'en'}}[$msgno]) {
	print STDERR "Warning: already had message $msgno for language 'en'\n";
    }
    ${$msgs{'en'}}[$msgno] = $msg;
    ++$n{$msgno};
}
my $last = 0;
for (sort { $a <=> $b } keys %n) {
    if ($_ > $last + 1) {
	print STDERR "Unused msg numbers: ", join(" ", $last + 1 .. $_ - 1), "\n";
    }
    $last = $_;
}
print STDERR "Last used msg number: $last\n";
%n = ();

for my $po_file (@ARGV) {
    my $language = $po_file;
    $language =~ s/\.po$//;

    my $po_hash = Locale::PO->load_file_ashash("$srcdir/$po_file");

    foreach my $po_entry (@{$num_list}) {
	my $msgno = $po_entry->dequote($po_entry->msgstr);
	next if $msgno !~ /^\d+$/;
	my $key = $po_entry->msgid;
	my $ent = $$po_hash{$key};
	if (defined $ent) {
	    my $msg = c_unescape($po_entry->dequote($ent->msgstr));
	    next if $msg eq '';
	    if (${$msgs{$language}}[$msgno]) {
		print STDERR "Warning: already had message $msgno for language $language\n";
	    }
	    ${$msgs{$language}}[$msgno] = $msg;
	}
    }

#       local $_;
#       open FINDUSES, "grep -no '*/$msgno\\>' \Q$srcdir\E/../src/*.cc \Q$srcdir\E../src/*.c \Q$srcdir\E/../src/*.h|" or die $!;
#       while (<FINDUSES>) {
#	   push @{$uses{$msgno}}, $1 if /^([^:]*:\d+):/;
#       }
#       close FINDUSES;

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
	       ++$missing;
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
      $buff .= $msg . "\0";
   }

   print OUT pack('N',length($buff)), $buff or die $!;
   close OUT or die $!;

   if ($missing || $retranslate) {
       print STDERR "Warning: ";
       if ($missing) {
	   print STDERR "$lang: $missing missing message(s)";
	   if ($retranslate) {
	       print STDERR " and $retranslate requiring retranslation";
	   }
       } else {
	   print STDERR "$lang: $retranslate message(s) requiring retranslation";
       }
       print STDERR "\n";
   }
}

sub sanity_check {
   my ($where, $msg, $orig) = @_;
   # FIXME: Only do this if the message has the "c-format" flag.
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

   # Check for missing (or added) ellipses (...)
   if ($msg =~ /\.\.\./ && $orig !~ /\.\.\./) {
       print STDERR "Warning: $where has ellipses but original doesn't\n";
   } elsif ($msg !~ /\.\.\./ && $orig =~ /\.\.\./) {
       print STDERR "Warning: $where is missing ellipses\n";
   }

   # Check for missing (or added) menu shortcut (@)
   if ($msg =~ /\@[A-Za-z]/ && $orig !~ /\@[A-Za-z]/) {
       print STDERR "Warning: $where has menu shortcut but original doesn't\n";
   } elsif ($msg !~ /\@[A-Za-z]/ && $orig =~ /\@[A-Za-z]/) {
       print STDERR "Warning: $where is missing menu shortcut\n";
   }

   # Check for missing (or added) menu shortcut (&)
   if ($msg =~ /\&[A-Za-z]/ && $orig !~ /\&[A-Za-z]/) {
       print STDERR "Warning: $where has menu shortcut but original doesn't\n";
   } elsif ($msg !~ /\&[A-Za-z]/ && $orig =~ /\&[A-Za-z]/) {
       print STDERR "Warning: $where is missing menu shortcut\n";
   }

   # Check for missing (or added) double quotes (“ and ”)
   if (scalar($msg =~ s/“/$&/g) != scalar($orig =~ s/(?:“|»)/$&/g)) {
       print STDERR "Warning: $where has different numbers of “\n";
       print STDERR "$orig\n$msg\n\n";
   }
   if (scalar($msg =~ s/”/$&/g) != scalar($orig =~ s/(?:”|«)/$&/g)) {
       print STDERR "Warning: $where has different numbers of ”\n";
       print STDERR "$orig\n$msg\n\n";
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

   # Check for missing (or added) menu accelerator "\t"
   if ($msg =~ /\t/ && $orig !~ /\t/) {
       print STDERR "Warning: $where has menu accelerator but original doesn't\n";
   } elsif ($msg !~ /\t/ && $orig =~ /\t/) {
       print STDERR "Warning: $where is missing menu accelerator\n";
   } elsif ($orig =~ /\t(.*)/) {
       my $acc_o = $1;
       my ($acc_m) = $msg =~ /\t(.*)/;
       if ($acc_o ne $acc_m) {
	   print STDERR "Warning: $where has menu accelerator $acc_m instead of $acc_o\n";
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
