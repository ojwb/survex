#!/usr/bin/perl -w
require 5.004;
use strict;

use integer;

my $version =`sed 's/^AM_INIT.*(.*, *\\([0-9][0-9\\.]*\\)).*/\\1/p;d' ../configure.in`;

my %email = (
 'de' => 'Gerhard Niklasch <nikl@mathematik.tu-muenchen.de>, <niklasch@consol.de>',
 'fr' => 'Eric.Madelaine@sophia.inria.fr',
 'it' => 'robywack@tin.it', # was zoom7@freemail.it
# 'es' => 'nobody@localhost',
 'ca' => 'jep@redestb.es',
 'pt' => 'leandro@dybal.eti.br',
 'sk' => 'martinsluka@mac.com'
);

#system "rm *.todo";

#system "./makemsgs.pl messages.txt";

for my $lang (sort keys %email) {
   #$email{$lang} = 'olly@mantis.co.uk';
   if (-s "$lang.todo") {
      print "Mailing $lang.todo to $email{$lang}\n";
      system "mutt -s 'Survex $version: Messages Requiring Translation' \Q$email{$lang}\E < \Q$lang.todo\E";
      unlink "$lang.todo";
   } else {
      print "$lang is up to date\n";
   }
}
