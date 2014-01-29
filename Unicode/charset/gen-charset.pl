#!/usr/bin/env perl

use strict;
use warnings;
use File::Temp qw/tempfile/;

my %charmaps = (
	'alphabet' => { 'get_map' => \&get_map_alpha, },
	'zapf'     => { 'get_map' => \&get_map_zapf , },
);

my ($maptype, $funcname, $infile, $outfile, $in_fh, $out_fh, @b2u, @u2b);

sub errmsg {
	print STDERR (shift)."\n";
	exit (shift);
}

sub usage {
	print <<_EOT_;
Generates character map files for use in FontForge.

Usage: $0 map_type short_name in_file [out_file]

If <out_file> is not specified, default is printing to standard output.

_EOT_
	exit 1;
}

sub get_map_alpha {
	while (<$in_fh>) {
		next if (/^#/ || /^\s*$/);
		if (! /^0x([[:xdigit:]]+)\s+0x([[:xdigit:]]+)\s/) {
			print STDERR "Unrecognised line format at line $.:\n";
			print STDERR "$_";
			next;
		}
		my ($b, $u) = map {hex} ($1, $2);
		print STDERR "Unicode code point beyond BMP at line $.\n" if ($u > 0xFFFF);
		print STDERR "Not a single byte character at line $.\n" if ($b > 0xFF);
		$b2u[$b] = $u;
		$u2b[$u >> 8][$u & 0xFF] = $b;
	}
}

sub get_map_zapf {
	while (<$in_fh>) {
		next if (/^#/ || /^\s*$/);
		if (! /^([[:xdigit:]]+)\s+([[:xdigit:]]+)\s/) {
			print STDERR "Unrecognised line format at line $.:\n";
			print STDERR "$_";
			next;
		}
		# column order for Zapf Dingbat file is oppose of all others
		my ($u, $b) = map {hex} ($1, $2);
		print STDERR "Unicode code point beyond BMP at line $.\n" if ($u > 0xFFFF);
		print STDERR "Not a single byte character at line $.\n" if ($b > 0xFF);

		# old table has 0x80 - 0x8D mapped to U+F8D7 - U+F8E4, but
		# since Unicode 3.2 they already have official code point
		# (U+2768 - U+2775)
		$u -= 0xD16F if (($u >= 0xF8D7) && ($u <= 0xF8E4));

		# 0x20 identity mapping has priority over U+A0 mapping
		$b2u[$b] = $u if (!$b2u[$b]);
		$u2b[$u >> 8][$u & 0xFF] = $b;
	}
}

sub print_header {
	print $out_fh <<_EOT_;
/* This file is generated for Fontforge use */
/* Please refer to Unicode/README.txt for detail */

#include <chardata.h>

static const unsigned char c_allzeros[256] = { 0 };

_EOT_
}

### Byte to Unicode part
sub print_b2u {
	print $out_fh "const unichar_t unicode_from_" . $funcname . "[] = {\n  ";
	printf $out_fh "0x%04x%s", $b2u[$_] // 0,
		($_ == 255)  ? "\n" :
		(($_+1) % 8) ? ", " : ",\n  " for (0 .. 255);
	print $out_fh "};\n\n";
}

#
# Theorecticall should split u2b and footer, but doing them
# together requires less code
#
sub print_u2b {
	my $first;
	my $last = $#u2b;
	my $s = $funcname;
	for (0 .. $last) {
		if (defined $u2b[$_]) {
			$first = $_; last;
		}
	}

	# each unicode block
	for my $block ($first .. $last) {
		next if (!defined $u2b[$block]);
		my $arrname = sprintf $s."_from_unicode_%x", $block;
		print $out_fh "static const unsigned char ".$arrname."[256] = {\n  ";
		printf $out_fh "0x%02x%s", $u2b[$block][$_] // 0,
			($_ == 255)   ? "\n" :
			(($_+1) % 16) ? ", " : ",\n  " for (0 .. 255);
		print $out_fh "};\n\n";
	}

	# map of unicode -> various byte arrays
	print $out_fh "static const unsigned char * const ".$s."_from_unicode_[] = {\n";
	for ($first .. $last) {
		if (!defined $u2b[$_]) {
			print $out_fh "    c_allzeros,\n";
		} else {
			printf $out_fh "    %s_from_unicode_%x%s\n", $s, $_, ($_ == $last) ? "" : ",";
		}
	}
	print $out_fh "};\n\n";

	# final part
	print $out_fh "struct charmap ".$s."_from_unicode = { ";
	printf $out_fh "%d, %d, ", $first, $last;
	print $out_fh "(unsigned char **) ".$s."_from_unicode_, (unichar_t *) unicode_from_".$s." };\n\n";
}

usage if ((@ARGV < 3) || (@ARGV > 4));
$maptype  = shift;
$funcname = shift;
$infile   = shift;

if (! $charmaps{$maptype}) {
	print STDERR "The map type '$maptype' is not supported. Supported types:\n";
	print STDERR "$_\n" foreach keys %charmaps;
	exit 2;
}

errmsg ("Input file '$infile' not found"     , 3) if (! -f $infile);
errmsg ("Can't open input file '$infile': $!", 3) if (! open($in_fh, '<', $infile));

$charmaps{$maptype}->{'get_map'}->();

close $in_fh;

if (@ARGV) {
	# tempfile will be renamed later
	($out_fh, $outfile) = tempfile( "charsetXXXXXX", UNLINK => 0 );
} else {
	$out_fh = *STDOUT;
}

print_header;
if (@b2u == 0) {
	unlink $outfile if $outfile;
	errmsg ("Failed to parse file", 4);
}
print_b2u;
print_u2b;

if (defined $outfile) {
	close $outfile;
	rename $outfile, shift;
}

# vim: sw=4 ts=4
