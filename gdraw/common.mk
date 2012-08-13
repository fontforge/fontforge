# This file is included by automake for "normal" builds, and by
# GNUmakefile in GNUstep builds.

GD_VERSION = 4
GD_REVISION = 10
GD_AGE = 0

GDRAW_C_SRCFILES = choosericons.c ctlvalues.c drawboxborder.c			\
	gaskdlg.c gbuttons.c gcolor.c gchardlg.c gcontainer.c gdraw.c		\
	gdrawbuildchars.c gdrawerror.c gdrawtxt.c gdrawtxtinit.c			\
	gfilechooser.c gfiledlg.c ggadgets.c ggroupbox.c gimageclut.c		\
	gimagecvt.c gimagepsdraw.c gimagewriteeps.c gdrawgimage.c			\
	gimagexdraw.c gkeysym.c glist.c gmenu.c gprogress.c gpsdraw.c		\
	gpstxtinit.c gradio.c gresource.c gresourceimage.c gresedit.c		\
	gsavefiledlg.c gscrollbar.c gtabset.c gtextfield.c gtextinfo.c		\
	gwidgets.c gxdraw.c gxcdraw.c ghvbox.c gmatrixedit.c gdrawable.c	\
	gspacer.c xkeysyms_unicode.c

GDRAW_H_SRCFILES = colorP.h gdrawP.h gimagebmpP.h gresourceP.h		\
	gxcdrawP.h fontP.h ggadgetP.h gpsdrawP.h gwidgetP.h gxdrawP.h
