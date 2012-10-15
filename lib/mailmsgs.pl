#!/usr/bin/perl -w
require 5.008;
use bytes;
use strict;

use integer;

my $version=`sed 's/^VERSION *= *//p;d' Makefile`;

my %email = (
 'de' => 'Gerhard Niklasch <nikl@mathematik.tu-muenchen.de>, <niklasch@consol.de>',
 'fr' => 'Eric.Madelaine@sophia.inria.fr',
 'it' => 'zoomx@tiscalinet.it', # was robywack@tin.it but that's webmail only
# 'es' => 'nobody@localhost',
 'ca' => 'josep@imapmail.org',
 'pt' => 'leandro@dybal.eti.br',
 'sk' => 'martinsluka@mac.com',
 'ro' => 'cristif@geo.unibuc.ro'
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
