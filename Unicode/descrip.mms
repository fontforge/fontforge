libgunicode_OBJECTS =  ArabicForms.obj,alphabet.obj,backtrns.obj,char.obj,\
	cjk.obj,memory.obj,ucharmap.obj,unialt.obj,ustring.obj,\
	utype.obj,usprintf.obj

CFLAGS=/incl=([-.inc])/name=(as_is,short)/nowarn/define=("NODYNAMIC=1")

all : [-.libs]libgunicode.olb
	write sys$output "unicode finished"

[-.libs]libgunicode.olb : $(libgunicode_OBJECTS)
	library/create [-.libs]libgunicode.olb $(libgunicode_OBJECTS)
