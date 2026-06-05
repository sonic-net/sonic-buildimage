#!/usr/bin/env perl
#
# $Id$
# $Copyright: (c) 2020 Broadcom.
# Broadcom Proprietary and Confidential. All rights reserved.$
#
# mktool.pl
#

use File::Path;
use File::Find;
use File::Copy;
use Cwd;

($prog = $0) =~ s/.*\///;

SWITCH:
{
    $op = shift;

    if ($op eq "-rm") { mktool_rm(@ARGV); last SWITCH; }
    if ($op eq "-cp") { mktool_cp(@ARGV); last SWITCH; }
    if ($op eq "-md") { mktool_md(@ARGV); last SWITCH; }
    if ($op eq "-ln") { mktool_ln(@ARGV); last SWITCH; }
    if ($op eq "-foreach") { mktool_foreach(@ARGV); last SWITCH; }
    if ($op eq "-dep") { mktool_makedep(@ARGV); last SWITCH; }
    if ($op eq "-echo") { mktool_echo(@ARGV); last SWITCH; }
    if ($op eq "-beep") { mktool_beep(@ARGV); last SWITCH; }
    die("$prog: unknown option '$op'\n");
}

exit 0;




#
# mktool_execute
#
# Executes a command, returns exist status. 
# Performs token special translation before execution. 
#

sub mktool_execute
{
    my $token = shift;
    my @cmds = @_;
    
#    printf("mktool_execute: token = '$token'\n");
    foreach $cmd (@cmds)
    {
	#printf("mktool_execute: cmd = '$cmd'\n");
	$cmd =~ s/\#\#/$token/g;
	if($cmd =~ /^-p/)
	{
	    $cmd =~ s/^-p//;
	    printf("$cmd\n");
	}
	else
	{
	    system($cmd);
	    my $excode = ($? >> 8);
	    exit $excode if $excode;
	}
    }
}


$find_regexp = "";
@find_cmd;
    
#
# mktool_foreach
#
sub mktool_foreach
{
    if($_[0] eq "-find")
    {
	shift;
	$find_dir = shift;
	$find_regexp = shift;
	@find_cmds = @_;
	
	if(!($find_dir =~ /^\//))
	{
	    $find_dir = cwd() . "/" . $find_dir;
	}
	find(\&_mktool_foreach_find_wanted, $find_dir);
    }
    else
    {
	my $subdir = 0;
	if($_[0] eq "-subdir")
	{
	    $subdir = 1;
	    shift;
	}

	my @thingies = split(' ', shift);
    
	foreach $thingy (@thingies)
	{
	    chdir $thingy unless $subdir == 0;
	    mktool_execute($thingy, @_);
	    chdir ".." unless $subdir == 0;
	}
    }
}



sub _mktool_foreach_find_wanted
{
    my $expr = "\$File::Find::name =~ /\^$find_regexp\$/";

    if(eval($expr))
    {
	mktool_execute($File::Find::name, @find_cmds);
	exit $excode if $excode;
    }
}


#
# rm
#
# Removes a list of objects
#
sub mktool_rm
{
    my($f);

    foreach $f (@_) {
	eval { rmtree($f) };
	if ($@) {
	    die "$prog $op: failed to remove $f: $@\n";
	}
    }
}

#
# md
#
# Makes a list of directories
#
sub mktool_md
{
    my($dir);

    foreach $dir (@_) {
	$dir =~ s!/+$!!;
	eval { mkpath($dir) };
	if ($@) {
	    die "$prog $op: failed to make directory $dir: $@\n";
        }
    }
}


sub mktool_cp
{
    my($from, $to) = @_;

    if (@_ != 2) {
	    die "$prog $op: must have two arguments\n";
    }
    copy($from, $to) ||
	die "$prog $op: failed to copy $from to $to: $!\n";
}

sub mktool_ln
{
    my($old, $new) = @_;

    if (@_ != 2) {
	    die "$prog $op: must have two arguments\n";
    }
    link ($old, $new) ||
	die "$prog $op: failed to link $new to $old: $!\n";
}


#	@echo "$@ \\" > ${BLDDIR}/$(notdir $@)
#	@if ($(DEPEND)) >> $(BLDDIR)/$(notdir $@); then	\
#	    exit 0;				\
#	else					\
#	    rm -f ${BLDDIR}/$(notdir $@);	\
#	    exit 1;				\
#	fi

#	$(MAKEDEP) "$@" "$(BLDDIR)/$(notdir $@)" "$(DEPEND)"

sub mktool_makedep
{
    my ($source, $target, $cmd) = @_;
    my $result = `$cmd`;
    if(!$?)
    {
	$result =~ s/^#.*\n//g;		# some makedeps produce comments
	$result =~ s/\n+$//;		# toss trailing newlines

	open (TARGET, ">$target") ||
	    die("$prog $op: cannot open '$target' for writing: $!\n");
	print TARGET "$source \\\n\${BLDDIR}/$result\n";
	close(TARGET);
    }
}

sub mktool_echo
{
    print "@_\n";
}

sub mktool_beep
{
    -t STDOUT && defined $ENV{MAKEBEEP} && print "\007";
}
