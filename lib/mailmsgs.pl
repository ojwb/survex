#!/usr/bin/perl -w
require 5.004;
use strict;

use integer;

my %email = (
 'de' => 'Gerhard Niklasch <nikl@mathematik.tu-muenchen.de>, gerhard.niklasch@okay.net',
 'fr' => 'Eric.Madelaine@sophia.inria.fr',
 'it' => 'zoom7@freemail.it',
 'es' => 'nobody@localhost',
 'ca' => 'jep@redestb.es',
 'pt' => 'leandro@dybal.eti.br'
);

system "rm *.todo";

system "./makemsgs.pl messages.txt";

for my $lang (sort keys %email) {
   #$email{$lang} = 'olly@mantis.co.uk';
   if (-s "$lang.todo") {
      print "Mailing $lang.todo to $email{$lang}\n";
      system "mail -s 'Survex: Messages Requiring Translation' \Q$email{$lang}\E < \Q$lang.todo\E";
      unlink "$lang.todo";
   } else {
      print "$lang is up to date\n";
   }
}
