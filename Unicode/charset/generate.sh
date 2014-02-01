#!/bin/bash

if [ "`which wget`" = "" ]; then
	echo "This script requires wget in PATH" 1>&2
	exit 1
fi

script="gen-charset.pl"

# If these files do change in future, command line arguments
# below may need changing as well
u="ftp://ftp.unicode.org/Public/MAPPINGS"
iso8859="$u/ISO8859/"
uni_maps=(
	"$u/OBSOLETE/EASTASIA/JIS/JIS0201.TXT"
	"$u/VENDORS/MISC/KOI8-R.TXT"
	"$u/VENDORS/APPLE/ROMAN.TXT"
	"$u/VENDORS/APPLE/SYMBOL.TXT"
	"$u/VENDORS/MICSFT/WINDOWS/CP1252.TXT"
	"$u/VENDORS/ADOBE/zdingbat.txt"
	"$u/VENDORS/MICSFT/WINDOWS/CP950.TXT"
)

a="ftp://ftp.oreilly.de/pub/examples/english_examples/nutshell/cjkv/adobe"
adobe_maps=(
#	"$a/ac15.tar.Z"		# Big5
	"$a/ag15.tar.Z"		# GB2312
	"$a/aj16.tar.Z"		# JIS X 0208
	"$a/aj20.tar.Z"		# JIS X 0212
	"$a/ak12.tar.Z"		# Johab & KSC 5601
)

if [ "$1" != "-s" ]; then		### shortcut to gen files only

[ ! -d "raw" ] && mkdir raw
cd raw

echo "===== Checking for updated character map files..."
wget --no-verbose --recursive --level=1 \
	--timestamping --no-directories --no-host-directories \
	$iso8859
wget --no-verbose --timestamping "${uni_maps[@]}"
wget --no-verbose --timestamping "${adobe_maps[@]}"

for i in "${uni_maps[@]}" "${adobe_maps[@]}"; do
	file=$(basename $i)
	[ -f "$file" ] || echo "Warning: $file not found" 1>&2
done

echo "===== Extracting data..."
for i in "${adobe_maps[@]}"; do
	code=$(basename $i .tar.Z)
	[ -d "$code" ] || tar zxf ${code}.tar.Z
	if [ ! -f "$code/cid2code.txt" ]; then
		echo "Warning: $code/cid2code.txt not found" 1>&2
	fi
done

cd ..

fi		### shortcut to gen files only

echo "===== Parsing character maps..."
error=()
while read line; do
	echo "===== Running $script $line ..."
	./$script $line
	if [ $? -ne 0 ]; then
		args=( $line )
		error[${#error[@]}]=${args[1]}
	fi
	echo
done > generate.log 2>&1 <<_EOT_
-o iso_8859_1.c default i8859_1 raw/8859-1.TXT
-o iso_8859_2.c default i8859_2 raw/8859-2.TXT
-o iso_8859_3.c default i8859_3 raw/8859-3.TXT
-o iso_8859_4.c default i8859_4 raw/8859-4.TXT
-o iso_8859_5.c default i8859_5 raw/8859-5.TXT
-o iso_8859_6.c default i8859_6 raw/8859-6.TXT
-o iso_8859_7.c default i8859_7 raw/8859-7.TXT
-o iso_8859_8.c default i8859_8 raw/8859-8.TXT
-o iso_8859_9.c default i8859_9 raw/8859-9.TXT
-o iso_8859_10.c default i8859_10 raw/8859-10.TXT
-o iso_8859_11.c default i8859_11 raw/8859-11.TXT
-o iso_8859_13.c default i8859_13 raw/8859-13.TXT
-o iso_8859_14.c default i8859_14 raw/8859-14.TXT
-o iso_8859_15.c default i8859_15 raw/8859-15.TXT
-o jis201.c default jis201 raw/JIS0201.TXT
-o koi8_r.c default koi8_r raw/KOI8-R.TXT
-o mac.c default mac raw/ROMAN.TXT
-o MacSymbol.c default MacSymbol raw/SYMBOL.TXT
-o win.c default win raw/CP1252.TXT
-o ZapfDingbats.c zapf ZapfDingbats raw/zdingbat.txt
-o big5.c big5 big5 raw/CP950.TXT
-o gb2312.c gb2312 gb2312 raw/ag15/cid2code.txt
-o jis.c jis aj16/cid2code.txt raw/aj20/cid2code.txt
-o johab.c johab johab raw/ak12/cid2code.txt
-o ksc5601.c wansung ksc5601 raw/ak12/cid2code.txt
_EOT_

if [ "${#error[@]}" -gt 0 ]; then
	echo "Following files fail to generate:"
	echo "${error[@]}"
	echo "Please refer to generate.log for more detail."
fi

