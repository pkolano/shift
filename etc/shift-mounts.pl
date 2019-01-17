#!/usr/bin/perl -T

# This program is a template that can be used to periodically collect file
# system information for Shift, which is used to determine file system
# equivalence for client spawns and transfer load balancing.

use strict;
use File::Temp;
use POSIX qw(setuid);

our $VERSION = 0.09;

# untaint PATH
$ENV{PATH} = "/bin:/usr/bin:/usr/local/bin";

############################
#### begin config items ####
############################

# user to use for ssh
#   (it is assumed this script will be invoked from root's crontab)
my $user = "someuser";

# set of hosts to collect mount information from
#   (it is assumed hostbased authentication can be used to reach all hosts)
#   (it is assumed shift-aux is in the default path on all hosts)
my @hosts = qw(
    host1 host2 ... hostN
);

# host where manager invoked
#   (it is assumed hostbased authentication can be used to reach this host)
#   (it is assumed shift-mgr is in the default path on this host)
my $mgr = "somehost";

##########################
#### end config items ####
##########################

# drop privileges and become defined user
my $uid = getpwnam($user);
setuid($uid) if (defined $uid);
die "Unable to setuid to $user\n"
    if (!defined $uid || $< != $uid || $> != $uid);

# create temporary file (automatically unlinked on exit)
my $tmp = File::Temp->new;
my $file = $tmp->filename;

# gather info from all hosts
foreach my $host (@hosts) {
    # use shift-aux to collect mount information and append to file
    open(FILE, "ssh -aqx -oHostBasedAuthentication=yes -oBatchMode=yes -l $user $host shift-aux mount |");
    while (<FILE>) {
        # print once for fully qualified host
        print $tmp $_;
        # ignore shell line for plain host
        next if (!/^args=/ || /^args=shell/);
        # replace fully qualified host with plain host
        s/(host=$host)\.\S+/$1/;
        # duplicate line for plain host
        print $tmp $_;
    }
    close FILE;
}
close $tmp;

# call shift-mgr to add collected info to global database
system("ssh -aqx -oHostBasedAuthentication=yes -oBatchMode=yes -l $user $mgr shift-mgr --mounts < $file");

