#!/usr/bin/perl -w
require 5.003;
use strict;

my $os = shift @ARGV;
my $configure_in = shift @ARGV;

my (%repl, $max_leaf_len, $use_rsp);

my $package = '';
my $version = '';
my $copyright = '';
if (open C, "<$configure_in") {
   while (<C>) {
      if (/^AM_INIT_AUTOMAKE\(\s*([^,]+?)\s*,\s*([\d.]+)\)/) {
         $package = $1;
	 $version = $2;
      } elsif (/^AC_DEFINE\(COPYRIGHT_MSG,/) {
         $_ = <C>;
	 ($copyright) = /(".*")/;
      }
   }
   close C;
   eval "&init_$os";
}

my $configh = shift @ARGV;
$@ = 1 if $version eq '' || $copyright eq '' || !defined $configh;
die "Syntax: $0 <platform> <configure.in> <config.h>\nSupported platforms: borlandc riscos\n" if $@;

open IN, "<$configh.in" or die $!;
open OUT, ">$configh.tmp" or die $!;
while (<IN>) {
   s/\@VERSION\@/$version/go;
   s/\@PACKAGE\@/$package/go;
   s/\@COPYRIGHT\@/$copyright/go;
   print OUT;
}
close IN;
close OUT;
unlink $configh;
rename "$configh.tmp", $configh or die $!;

my %var = ();

while (<>) {
   while (s/\\\n$//) {
      $_ .= <>;
   }
   
   repl(\$_);
   
   if (/^(\w+)\s*=\s*(.*)$/) {
      if (defined $var{$1}) {
         $var{$1} .= " " . $2;
      } else {
	 $var{$1} = $2;
      }
   }
}

sub shorten {
   my $exe = shift;
   my $max_len = shift;
   while (length $exe > $max_len) {
      unless ($exe =~ s/[aeiou]//i) {
         $exe =~ s/.$//;
      }
   }
   return $exe;
}

my $progs = $var{'bin_PROGRAMS'};
$progs =~ s|\$\((\w+)\)|$var{$1}|eg;

my $t = join " ", map {shorten($_, $max_leaf_len)} split /\s+/, $progs;
$t =~ s/(\w)\b/$1$repl{'EXEEXT'}/g if $repl{'EXEEXT'};
print "all: $t\n\n";

print ".c.$repl{'OBJEXT'}:\n";
print "\t$repl{'CC'} $repl{'CFLAGS'} -c \$<\n\n";

print $repl{'EXTRARULES'};
		
for (sort keys %var) {
   if (/^(\w+)_SOURCES/) {
      my $prog = $1;      
      if ($progs =~ /\b\Q$prog\E\b/) {
         my $exe = shorten($prog, $max_leaf_len) . $repl{'EXEEXT'};

         my $sources = $var{$_};

         my $ldadd = $var{$prog.'_LDADD'} || $var{'LDADD'} || '';
         my $extra;

#	 $extra = $var{"EXTRA_$_"};
#	 $sources .= " " . $extra if defined $extra;

#         $extra = $repl{"LIBOBJS"};
#	 $sources .= " " . $extra if defined $extra;

	 $sources =~ s|\$\((\w+)\)|$var{$1}|eg;
	 my $objs = $sources;
	 $objs =~ s/\.[cs]\b/.$repl{'OBJEXT'}/g;
	 # expand vars
	 s|\$\((\w+)\)|$var{$1}|eg;
	 print "$exe: $objs";
	 my $x = join " ", grep /\.$repl{'OBJEXT'}$/, split /\s+/, $ldadd;
	 print " $x" if $x ne '';
	 print "\n";
	 if ($use_rsp) {
	    my $rsp = "t.rsp";
	    my $len = 55;
	    my $redir = ">";	 
	    while (length $objs > $len) {
	       $objs =~ s|^(.{1,$len})\s||o;
	       print "\t\@echo $1 $redir $rsp\n";
	       $redir = ">>";
	    }
	    print "\t\@echo $objs $redir $rsp\n"; # FIXME: @ is msdos specific
	    print "\t$repl{'CC'} -e$exe";
	    print " $repl{'CFLAGS'}" if $repl{'CFLAGS'} ne '';
	    print " $ldadd" if $ldadd ne '';
	    print " $repl{'LDFLAGS'}" if $repl{'LDFLAGS'} ne '';
	    print " \@$rsp";
	    print " $repl{'LIBS'}" if $repl{'LIBS'} ne '';
	    print "\n";
	    print "\t\@del /q $rsp\n"; # FIXME: msdos specific
	 } else {
	    print "\t$repl{'CC'} -o $exe";
	    print " $repl{'CFLAGS'}" if $repl{'CFLAGS'} ne '';
	    print " $ldadd" if $ldadd ne '';
	    print " $repl{'LDFLAGS'}" if $repl{'LDFLAGS'} ne '';
	    print " $objs";
	    print " $repl{'LIBS'}" if $repl{'LIBS'} ne '';
	    print "\n";
	    print "\tsqueeze $exe\n"; # FIXME: riscos specific
	 }
	 print "\n";
      }      
   }
}

sub repl {
   my $ref = shift;
   $$ref =~ s!\@(\w+)\@!repl_func($1)!eg;
}

sub repl_func {
   my $x = shift;
   return $repl{$x} if exists $repl{$x};
   print STDERR "No replacement for \@${x}\@\n";
   return '';
}

sub init_riscos {
   %repl = (
      'CAVEROT' => 'caverot',
      'LIBOBJS' => 'strcasecmp.o',
      'CRLIB' => '',
      'CROBJX' => 'armrot.o',
      'CFLAGS' => '-DHAVE_CONFIG_H -IC:,@ -throwback -ffahp -fussy',
      'LDFLAGS' => '',
      'LIBS' => 'C:OSLib.o.OSLib',
      'CC' => 'cc',
      'OBJEXT' => 'o',
      'EXEEXT' => '',
      'EXTRARULES' => 
         "armrot.o: armrot.s\n\tobjasm -ThrowBack -Stamp -quit -CloseExec -from s.armrot -to o.armrot\n\n",	 
   );
   $max_leaf_len = 10;
   $use_rsp = 0;
}

sub init_borlandc {
   %repl = (
      'CAVEROT' => 'caverot',
      'LIBOBJS' => '',
      'CRLIB' => 'graphics.lib mouse.lib',
      'CROBJX' => 'dosrot.obj',
      'CFLAGS' => '-DHAVE_CONFIG_H -DHAVE_FAR_POINTERS -I. -ml -d -O1 -Ogmpvl -X',
      'LDFLAGS' => '',
      'LIBS' => '',
      'CC' => 'bcc',
      'OBJEXT' => 'obj',
      'EXEEXT' => '.exe',
      'EXTRARULES' => '',
   );
   $max_leaf_len = 8;
   $use_rsp = 1;
}
