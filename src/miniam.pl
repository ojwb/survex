#!/usr/bin/perl -w
require 5.003;
use strict;

my $os = shift @ARGV;

my (%repl, $max_leaf_len, $use_rsp);

eval "&init_$os";

die "Syntax: $0 <platform>\nSupported platforms: borlandc riscos\n" if $@;
my %var = ();

while (<>) {
   while (s/\\\n$//) {
      $_ .= <>;
   }
   
   s!\@(\w+)\@!$repl{$1}||"\@$1\@"!eg;
   
   if (/^(\w+)\s*=\s*(.*)$/) {
      $var{$1} = $2;
   }
}

my $progs = $var{'bin_PROGRAMS'};
$progs =~ s|\$\((\w+)\)|$var{$1}|eg;
my $t = $progs;
$t =~ s/(\w)\b/$1$repl{'EXEEXT'}/g if $repl{'EXEEXT'};
print "all: $t\n\n";

print ".c.$repl{'OBJEXT'}:\n";
print "\t$repl{'CC'} $repl{'CFLAGS'} -c \$<\n\n";

print $repl{'EXTRARULES'};
		
for (sort keys %var) {
   if (/^(\w+)_SOURCES/) {
      my $prog = $1;      
      if ($progs =~ /\b\Q$prog\E\b/) {
         my $exe = $prog;
	 while (length $exe > $max_leaf_len) {
	    unless ($exe =~ s/[aeiou]//i) {
	       $exe =~ s/.$//;
	    }
	 }
	 $exe .= $repl{'EXEEXT'};

         my $sources = $var{$_};

	 my $extra = $var{"EXTRA_$_"};
	 $sources .= " " . $extra if defined $extra;

         $extra = $repl{"LIBOBJS"};
	 $sources .= " " . $extra if defined $extra;

	 $sources =~ s|\$\((\w+)\)|$var{$1}|eg;
	 my $objs = $sources;
	 $objs =~ s/\.[cs]\b/.$repl{'OBJEXT'}/g;
	 # expand vars
	 s|\$\((\w+)\)|$var{$1}|eg;
	 print "$exe: $objs\n";
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
	    print "\t$repl{'CC'} -e$exe $repl{'LDFLAGS'} \@$rsp\n";
	    print "\t\@del /q $rsp\n"; # FIXME: msdos specific
	 } else {
	    print "\t$repl{'CC'} -o $exe $repl{'LDFLAGS'} $objs $repl{'LIBS'}\n";
	    print "\tsqueeze $exe\n"; # FIXME: riscos specific
	 }
	 print "\n";
      }      
   }
}


sub init_riscos {
   %repl = (
      'CAVEROT' => 'caverot',
      'LIBOBJS' => '',
      'CRLIB' => 'graphics.lib mouse.lib',
      'CROBJX' => 'dosrot.c',
      'CFLAGS' => '-DHAVE_CONFIG_H -DHAVE_FAR_POINTERS -I. -ml -d -O1 -Ogmpvl -X',
      'LDFLAGS' => '-lm',
      'LIBS' => '',
      'CC' => 'cc',
      'OBJEXT' => 'o',
      'EXEEXT' => '',
      'EXTRARULES' => 
         "armrot.o: armrot.s:\n\tobjasm -ThrowBack -Stamp -quit -CloseExec -from s.armrot -to o.armrot\n\n",	 
   );
   $max_leaf_len = 10;
   $use_rsp = 0;
}

sub init_borlandc {
   %repl = (
      'CAVEROT' => 'caverot',
      'LIBOBJS' => 'strcasecmp.c',
      'CRLIB' => '',
      'CROBJX' => 'armrot.s',
      'CFLAGS' => '-DHAVE_CONFIG_H -IC: -throwback -ffahp -fussy',
      'LDFLAGS' => '',
      'LIBS' => 'C:OSLib.o.OSLib',
      'CC' => 'bcc',
      'OBJEXT' => 'obj',
      'EXEEXT' => '.exe',
      'EXTRARULES' => '',
   );
   $max_leaf_len = 8;
   $use_rsp = 1;
}
