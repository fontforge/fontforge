libgunicode_OBJECTS =  ArabicForms.obj,alphabet.obj,backtrns.obj,char.obj,\
	cjk.obj,memory.obj,ucharmap.obj,unialt.obj,ustring.obj,\
	utype.obj,usprintf.obj,gwwiconv.obj

CFLAGS=/incl=([-.inc])/name=(as_is,short)/nowarn/define=(\
	"_STATIC_LIBFREETYPE=1","_STATIC_LIBPNG=1","HAVE_LIBINTL_H=1",\
	"_STATIC_LIBUNINAMESLIST=1","_STATIC_LIBXML=1","_NO_XINPUT=1",\
	"_STATIC_LIBUNGIF=1","_STATIC_LIBJPEG=1","_STATIC_LIBTIFF=1")

all : [-.libs]libgunicode.olb
	write sys$output "unicode finished"

[-.libs]libgunicode.olb : $(libgunicode_OBJECTS)
	library/create [-.libs]libgunicode.olb $(libgunicode_OBJECTS)

ArabicForms.obj : ArabicForms.c
alphabet.obj : alphabet.c
backtrns.obj : backtrns.c
char.obj : char.c
cjk.obj : cjk.c
memory.obj : memory.c
ucharmap.obj : ucharmap.c
unialt.obj : unialt.c
ustring.obj : ustring.c
utype.obj : utype.c
usprintf.obj : usprintf.c
gwwiconv.obj : gwwiconv.c
