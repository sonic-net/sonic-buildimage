#!/usr/bin/perl
use warnings;
use strict;
use Debian::Debhelper::Dh_Lib;

insert_after('dh_shlibdeps', 'dh_kerneldeps');

1;
