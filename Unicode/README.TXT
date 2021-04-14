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
gcc -s -I../inc -o makebuildtables makebuildtables.c
./makebuildtables
rm *.txt
mv utype.h ../inc/
