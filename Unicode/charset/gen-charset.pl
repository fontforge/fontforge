#!/usr/bin/env perl

###########################################################
#
# This script reads a raw character map file and dump corresponding
# character map C source file, for inclusion in FontForge. Currently
# supports Adobe CID mapping file, Unicode.org table Format A, as
# well as specific table format for Zapf Dingbats.
#
# Although this script tries to emulate same function as
# Unicode/dump.c in older versions of FontForge, it is not a 100%
# faithful translation, especially in the character selection process
# when multiple character correspond to a single Adobe CID value. As
# a result, some ambiguous characters may gone missing (instead of
# arbitrarily picking one).
#
###########################################################
#
# Copyright (C) 2014 Abel Cheung
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# The name of the author may not be used to endorse or promote products
# derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

use strict;
use warnings;
use File::Temp qw/tempfile/;
use Getopt::Std;
use constant VERTICAL => 1<<31;

my @size = ("char", "short");
my @struct = ("charmap", "charmap2");

# For adobe CMap, column count starts from 0
my %charmaps = (
	'default' => {
		'get_map'      => \&get_map_default,
		'write_val'    => \&write_offset,    # no offset ( = 0 )
	},
	'zapf'    => {	# VENDORS/ADOBE/zdingbat.txt
		'get_map'      => \&get_map_zapf,
	},
	'big5'    => {	# VENDORS/MICSFT/WINDOWS/CP950.TXT
		'get_map'      => \&get_map_default,
		'write_val'    => \&write_offset,    'offset' => 0xA100,
	},
	'jis'     => {},
	'jis208'  => {	# aj16/cid2code.txt
		'get_map'      => \&get_map_adobe,   'bcol' =>  1, 'ucol' => 21,
		'write_val'    => \&write_compact,
	},
	'jis212'  => {	# aj20/cid2code.txt
		'get_map'      => \&get_map_adobe,   'bcol' =>  1, 'ucol' => 6,
		'write_val'    => \&write_compact,
	},
	'gb2312'  => {	# ag15/cid2code.txt
		'get_map'      => \&get_map_adobe,   'bcol' =>  1, 'ucol' => 13,
		'write_val'    => \&write_compact,
	},
	'wansung' => {	# ak12/cid2code.txt
		'get_map'      => \&get_map_adobe,   'bcol' =>  1, 'ucol' => 10,
		'write_val'    => \&write_compact,
	},
	'johab'   => {	# ak12/cid2code.txt
		'get_map'      => \&get_map_adobe,   'bcol' =>  6, 'ucol' => 10,
		'write_val'    => \&write_offset,    'offset' => 0x8400,
	},
);

my ($maptype, $funcname, $infile, $outfile, $out_fh, @b2u, @u2b, %opts);
my $is_mb = 0;

sub errmsg {
	print STDERR (shift)."\n";
	if (defined $outfile) {
		close $out_fh;
		unlink $outfile;
	}
	exit (shift);
}

sub usage {
	print <<_EOT_;
Generates character map files for use in FontForge.

Usage: $0 [-o out_file] map_type short_name in_file

If <out_file> is not specified, default is printing to standard output.
When <map_type> is 'jis', different invoking arguments will be used,
simultaneously reading two files:

       $0 [-o out_file] jis in_file1 in_file2

_EOT_
	exit 1;
}

# Corresponds to 'Format A' table in Unicode mirror
# 2 columns: (byte value) (unicode code point)
sub get_map_default {
	my ($file, $barray, $uarray) = @_;
	errmsg ("Can't open input file '$file': $!", 3) if (! open( my $fh, '<', $file ));

	while (<$fh>) {
		next if (/^#/ || /^\s*$/);
		if (! /^0x([[:xdigit:]]+)\s+0x([[:xdigit:]]+)\s/) {
			print STDERR "Unrecognised line format at line $.:\n";
			print STDERR "$_";
			next;
		}
		my ($b, $u) = map {hex} ($1, $2);
		$charmaps{$maptype}->{'write_val'}->($b, $u, $barray, $uarray);
	}
	close $fh;
	return ((@{$barray} > 0) && (@{$uarray} > 0));
}

sub get_map_zapf {
	my ($file, $barray, $uarray) = @_;
	errmsg ("Can't open input file '$file': $!", 3) if (! open( my $fh, '<', $file ));

	while (<$fh>) {
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
		$barray->[$b] = $u if (!$b2u[$b]);
		$uarray->[$u >> 8][$u & 0xFF] = $b;
	}
	close $fh;
	return ((@{$barray} > 0) && (@{$uarray} > 0));
}

# algorithm mainly from KANOU Hiroki
sub ucs2_score {
	my ($chr) = @_;
	return
		(($chr >= 0xe000) && ($chr <= 0xf8ff)) ? -1 :
		(($chr >= 0x2e80) && ($chr <= 0x2fff)) ? 1 :
		($chr & VERTICAL)                      ? 0 :
		(($chr >= 0xf000) && ($chr <= 0xffff)) ? 3 :
		(($chr >= 0x3400) && ($chr <= 0x4dff)) ? 4 : 5;
}

# selects *first* value in array having highest ucs2_score
sub pick_value {
	my ($score, $best) = (0, 0);
	my @vals = split /,/, $_[0];
	foreach (@vals) {
		next if /v/;	# all vertical glyphs are discarded
		my $chr = hex($_);
		if (ucs2_score($chr) > $score) {
			$score = ucs2_score($chr);
			$best = $chr;
		}
	}
	return $best;
}

# how many values are non-vertical hex digits?
sub eligible {
	my @vals = split /,/, shift;
	my $count = 0;
	foreach (@vals) {
		$count++ if ($_ =~ /^[[:xdigit:]]+$/);
	}
	return $count;
}

sub get_map_adobe {
	my ($file, $barray, $uarray, $flag, $dummy) = @_;
	my (@cols, $bcol, $ucol, $cid);

	$bcol = $charmaps{$maptype}->{'bcol'};
	$ucol = $charmaps{$maptype}->{'ucol'};

	return if (($bcol <= 0) || ($ucol <= 0));	# 0th column is CID
	errmsg ("Can't open input file '$file': $!", 3) if (! open( my $fh, '<', $file ));

	# for safety: how many columns are there in the file?
	while (<$fh>) {
		next if (!/^\d/);
		@cols = split;
		last;
	}
	return if (($bcol > @cols) || ($ucol > @cols));

	seek($fh, 0, 0);

	while (<$fh>) {
		next if (!/^\d/);
		@cols = split /\s+/;
		my ($b, $u, $cid) = ($cols[$bcol], $cols[$ucol], $cols[0]);

		# Discard when:
		# byte value or unicode code point doesn't exist
		# byte value indicates glyph is in vertical form
		# byte value is not unique
		next if (($b =~ /\*/) || ($u =~ /\*/));
		if ($b !~ /[,v]/) {
			$b = hex($b);
		} else {
			my $c = eligible($b);
			if (!$c) {
				print STDERR "No eligible character for CID $cid, skipped\n";
				next;
			} elsif ($c > 1) {
				print STDERR "Multiple possible character for CID $cid, skipped\n";
				next;
			}
			$b = pick_value($b);
		}

		if ($u !~ /[,v]/) {
			$u = hex($u);
		} else {
			# IMO this arbitrary character selection process is retarded,
			# but for compatibility... sigh
			$u = pick_value($u);
		}
		next if (!$u);

		$charmaps{$maptype}->{'write_val'}->($b, $u, $barray, $uarray, $flag);
	}
	close $fh;
	return ((@{$barray} > 0) && (@{$uarray} > 0));
}

# byte values for some CJK charsets are compacted into 94x94 array
# before writing
sub write_compact {
	my ($b, $u, $barray, $uarray, $flag_jis212, $dummy) = @_;

	if ((($b>>8) < 0x21) || (($b & 0xff) < 0x21) ||
	    (($b>>8) > 0x7e) || (($b & 0xff) > 0x7e)) {
		printf STDERR "Byte value out of bound: byte = %04x, unicode = %04x\n", $b, $u;
		return;
	}
	if ($u > 0xffff) {
		printf STDERR "Unicode beyond BMP: byte = %04x, unicode = %04x\n", $b, $u;
		return;
	}
	if ($flag_jis212) {
		if ($uarray->[$u >> 8][$u & 0xff]) {
			printf STDERR "Unicode value already defined: unicode = %04x, orig byte = %04x, new = %04x\n", $u, $uarray->[$u>>8][$u&0xff], $b;
		} else {
			$uarray->[$u >> 8][$u & 0xff] = ($b | 0x8000);
		}
	} else {
		$uarray->[$u >> 8][$u & 0xff] = $b;
	}
	$b -= 0x2121;
	$b = ($b>>8) * 94 + ($b & 0xff);
	$barray->[$b] = $u;
}

# big5, johab etc: some multibyte arrays are just dumped in straight
# forward way, minus certain offset
sub write_offset {
	my ($b, $u, $barray, $uarray, $dummy) = @_;

	if ($u > 0xffff) {
		printf STDERR "Unicode beyond BMP: byte = %04x, unicode = %04x\n", $b, $u;
		return;
	}
	if (my $o = $charmaps{$maptype}->{'offset'}) {
		if ($b < $o) {
			printf STDERR "Byte value is smaller than offset: %04x\n", $b;
			return;
		}
		$barray->[$b-$o] = $u;
	} else {
		$barray->[$b] = $u;
	}
	$uarray->[$u >> 8][$u & 0xff] = $b;
}

sub print_header {
	print $out_fh <<_EOT_;
/* This file is generated for Fontforge use */
/* Please refer to Unicode/README.txt for detail */

#include "chardata.h"

_EOT_
	print $out_fh "static const unsigned " . $size[$is_mb] .
		" allzeros[256] = { 0 };\n\n";
}

### Byte to Unicode part
sub print_b2u {
	my ($barray, $name) = @_;

	my $last = $is_mb ? $#{$barray} : 255;	# always dump 256 element for alphabet charset
	my $o = $charmaps{$maptype}->{'offset'} // 0;

	print $out_fh "const unichar_t unicode_from_".$name."[".($last+1)."] = {\n  ";
	for (0 .. $last) {
		($_ % 64) or printf $out_fh "/*** 0x%04x ***/\n  ", $_ + $o;
		printf $out_fh "0x%04x%s", $barray->[$_] // 0,
			($_ == $last) ? "\n" :
			(($_+1) % 8 ) ? ", " : ",\n  ";
	}
	print $out_fh "};\n\n";
}

# Theorecticall should split u2b and footer, but doing them
# together requires less code
#
sub print_u2b {
	my ($uarray, $name, $name2) = @_;

	$name2 = $name if (!$name2);
	my $first;
	my $last = $#{$uarray};

	for (0 .. $last) {
		if (defined $uarray->[$_]) {
			$first = $_; last;
		}
	}

	# each unicode block
	for my $block ($first .. $last) {
		next if (!defined $uarray->[$block]);
		my $arrname = sprintf $name2 . "_from_unicode_%x", $block;
		print $out_fh "static const unsigned " .
			$size[$is_mb] . " " . $arrname . "[256] = {\n  ";
		for (0 .. 255) {
			($_ % 64) or printf $out_fh "/*** 0x%02x ***/\n  ", $_;
			printf $out_fh ($is_mb ? "0x%04x%s" : "0x%02x%s"), $uarray->[$block][$_] // 0,
				($_ == 255)  ? "\n" :
				(($_+1) % ($is_mb?8:16)) ? ", " : ",\n  ";
		}
		print $out_fh "};\n\n";
	}

	# map of unicode -> various byte arrays
	print $out_fh "static const unsigned " .
		$size[$is_mb] . " * const " . $name2 . "_from_unicode_[] = {\n";
	for ($first .. $last) {
		if (!defined $uarray->[$_]) {
			print $out_fh "    allzeros,\n";
		} else {
			printf $out_fh "    %s_from_unicode_%x%s\n", $name2, $_, ($_ == $last) ? "" : ",";
		}
	}
	print $out_fh "};\n\n";

	# final statement
	print $out_fh "struct " . $struct[$is_mb] . " " . $name2 . "_from_unicode = { ";
	printf $out_fh "%d, %d, ", $first, $last;
	print $out_fh "(unsigned " . $size[$is_mb] . " **) " . $name2 .
		"_from_unicode_, (unichar_t *) unicode_from_" . $name . " };\n\n";
}

getopts('o:', \%opts);
$maptype  = shift;

if (! $charmaps{$maptype}) {
	print STDERR "The map type '$maptype' is not supported. Supported types:\n";
	print STDERR "$_\n" foreach keys %charmaps;
	exit 2;
}

# no reliable way to check multibyte w/o reading files, so define manually
$is_mb = 1 if grep (/^$maptype$/, ('big5', 'gb2312', 'jis', 'jis208', 'jis212', 'johab', 'wansung'));

if ($opts{'o'}) {
	# tempfile will be renamed later
	($out_fh, $outfile) = tempfile( "tmpcharsetXXXXXX", UNLINK => 0 );
} else {
	$out_fh = *STDOUT;
}

if ( $maptype eq "jis" ) {
	usage if (@ARGV != 2);

	# Print only the byte->unicode part of JIS X 0208
	$maptype = "jis208";
	$infile = shift;
	if (! $charmaps{$maptype}->{'get_map'}->( $infile, \@b2u, \@u2b ) ) {
		errmsg ("Failed to parse file '$infile'", 4);
	}
	print_header;
	print_b2u (\@b2u, "jis208");

	# And then JIS X 0212, as well as combined unicode->byte
	# array of both
	$maptype = "jis212";
	$infile = shift;
	undef @b2u;
	# reuse @u2b; extra argument indicating all byte values of JIS X 0212 
	# will be or'ed with 0x8000 before writing
	if (! $charmaps{$maptype}->{'get_map'}->( $infile, \@b2u, \@u2b, 1 ) ) {
		errmsg ("Failed to parse file '$infile'", 4);
	}
	print_b2u (\@b2u, "jis212");
	print_u2b (\@u2b, "jis212", "jis");

} else {
	usage if (@ARGV != 2);
	($funcname, $infile) = (shift, shift);
	if (! $charmaps{$maptype}->{'get_map'}->( $infile, \@b2u, \@u2b ) ) {
		errmsg ("Failed to parse file '$infile'", 4);
	}
	print_header;
	print_b2u (\@b2u, $funcname);
	print_u2b (\@u2b, $funcname);
}

if (defined $outfile) {
	close $outfile;
	rename $outfile, $opts{'o'};
}

# vim: sw=4 ts=4 fdn=1 fdm=indent fml=5
