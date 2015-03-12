#!/usr/bin/perl -w

use strict;
use IO::File;
use Time::HiRes qw ( setitimer ITIMER_VIRTUAL ITIMER_REAL time );
use File::Basename;
my $dirname = dirname(__FILE__);

our $COCOA_DIALOG = "$dirname/CocoaDialog.app/Contents/MacOS/CocoaDialog";
die "$COCOA_DIALOG doesn't exist" unless -e $COCOA_DIALOG;

my $forcecache = "";
if ( ! -d "/Applications/FontForge.app/Contents/Resources/opt/local/var/cache/fontconfig" ) {
    $forcecache = "-f";
} 
my $fcproc = IO::File->new("$dirname/fc/fc-cache $forcecache  -v /Applications/FontForge.app/Contents/Resources/opt/local/share/fontforge/pixmaps |");

my $bodypreamble = "This setup is only run once...";
my $args = '--title "FontForge is scanning for fonts" --text "This setup is only run once..."';
my $fh;

my $msg = 1;
my $line = "";
$/=":";

$SIG{ALRM} = sub { 
    print "alarm!";
    print "$msg $line fc-cache\n";
    if( not defined($fh)) {
      $fh = IO::File->new("|$COCOA_DIALOG progressbar --indeterminate $args");
      die "no fh" unless defined $fh;
      $fh->autoflush(1);
    }
    print $fh "$msg $bodypreamble $line\n"; 
};
setitimer(ITIMER_REAL, 1, 1);

while( <$fcproc> ) {
    
    s/[^\n]*\n//g;
    s/://g;
    $line = $_;
    print "--> $line <--\n";
    $msg = $msg + 1;
}

print "Done!\n\n";
if(defined($fh)) { $fh->close(); }
$fcproc->close();

