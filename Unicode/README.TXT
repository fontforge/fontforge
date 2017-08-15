UPDATING FONTFORGE'S SUPPORT FOR UNICODE

Thanks to Khaled Hosny for reverse-engineering this.
To update unicode coverage, run:

cd Unicode
wget http://unicode.org/Public/UNIDATA/LineBreak.txt
wget http://unicode.org/Public/UNIDATA/NamesList.txt
wget http://unicode.org/Public/UNIDATA/PropList.txt
wget http://unicode.org/Public/UNIDATA/UnicodeData.txt
gcc -s -I../inc -o makeutype makeutype.c
./makeutype
rm *.txt
mv utype.h ../inc/

UPDATING FONTFORGE'S ROUTINES FOR RE-ENCODING

There is a little program dump.c to update reencoding tables. It needs some
external files to work. Here is my best attempt to find these files. You will
notice that some files are still lacking. I couldn't find them.

Thanks to Jose Da Silva for asking, what's it for. To make use of this, run:

cd Unicode
wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-1.TXT
wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-2.TXT
wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-3.TXT
wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-4.TXT
wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-5.TXT
wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-6.TXT
wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-7.TXT
wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-8.TXT
wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-9.TXT
wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-10.TXT
wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-11.TXT
wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-13.TXT
wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-14.TXT
wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-15.TXT
wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-16.TXT
wget http://unicode.org/Public/MAPPINGS/VENDORS/MISC/KOI8-R.TXT
wget http://unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/JIS/JIS0201.TXT
wget http://unicode.org/Public/MAPPINGS/VENDORS/ADOBE/zdingbat.txt
wget http://unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/JIS/JIS0212.TXT
wget http://unicode.org/Public/MAPPINGS/VENDORS/APPLE/ROMAN.TXT
wget http://unicode.org/Public/MAPPINGS/VENDORS/APPLE/SYMBOL.TXT
wget http://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1252.TXT
wget http://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP950.TXT
wget http://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/MAC/CYRILLIC.TXT
mv CYRILLIC.TXT MacCYRILLIC.TXT

gcc -s -I../inc -o dump dump.c
./dump
rm *.txt *.TXT

NOTE: Some standards listed above may be obsolete, check for updates by reading
ftp://ftp.ora.com/pub/examples/nutshell/ujip/doc/cjk.inf
ftp://ftp.ora.com/pub/examples/nutshell/ujip/00README

