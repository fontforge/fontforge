#!/bin/tcsh

if ( $#argv < 1 ) then
  echo "usage: $0 <CLDR main directory>"
  echo "example: $0 ~/Desktop/common/main"
  exit 1
endif

set cldr=$argv[1]
if ( ! -d "$cldr" ) then
  echo "error: CLDR directory not found: $cldr"
  exit 1
endif

set script_dir=`dirname "$0"`
set repo_root=`cd "$script_dir/../.." && pwd`

set csv=$repo_root/po/tools/msgids.csv
set en=$cldr/en.xml

if ( ! -f "$csv" ) then
  echo "error: CSV not found: $csv"
  exit 1
endif

if ( ! -f "$en" ) then
  echo "error: English CLDR XML not found: $en"
  exit 1
endif

mkdir -p $repo_root/lang

foreach loc (`grep -v '^[[:space:]]*$' $repo_root/po/LINGUAS`)
  set po_in=$repo_root/po/$loc.po
  set out=$repo_root/lang/$loc.po
  set locxml=""

  if ( -f $cldr/$loc.xml ) then
    set locxml=$cldr/$loc.xml
  else if ( -f $cldr/`echo $loc | tr '_' '-'`.xml ) then
    set locxml=$cldr/`echo $loc | tr '_' '-'`.xml
  else if ( -f $cldr/`echo $loc | sed 's/_.*//'`.xml ) then
    set locxml=$cldr/`echo $loc | sed 's/_.*//'`.xml
  endif

  if ( "$loc" == "zh_CN" && -f $cldr/zh.xml ) set locxml=$cldr/zh.xml
  if ( "$loc" == "zh_TW" && -f $cldr/zh_Hant.xml ) set locxml=$cldr/zh_Hant.xml
  if ( "$loc" == "sv_SE" && -f $cldr/sv.xml ) set locxml=$cldr/sv.xml
  if ( "$loc" == "th_TH" && -f $cldr/th.xml ) set locxml=$cldr/th.xml
  if ( "$loc" == "tr_TR" && -f $cldr/tr.xml ) set locxml=$cldr/tr.xml

  if ( ! -f $po_in ) then
    echo "skip $loc: missing $po_in"
    continue
  endif

  if ( "$locxml" == "" ) then
    echo "skip $loc: no CLDR XML found"
    continue
  endif

  if ( -f "$out" ) then
    rm -f "$out"
  endif

  echo "processing $loc with $locxml"
  python3 $repo_root/po/tools/build_lang_po_from_cldr.py "$csv" "$en" "$po_in" "$locxml" -o "$out"
end
