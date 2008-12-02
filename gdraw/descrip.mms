# Makefile for OpenVMS
# Date : 28 April 2008

libgdraw_OBJECTS =  choosericons.obj,drawboxborder.obj,\
 gaskdlg.obj,gbuttons.obj,gchardlg.obj,gcontainer.obj,gdraw.obj,\
 gdrawbuildchars.obj,gdrawerror.obj,gdrawtxt.obj,gdrawtxtinit.obj,\
 genkeysym.obj,gfilechooser.obj,gfiledlg.obj,ggadgets.obj,ggroupbox.obj,\
 gimageclut.obj,gimagecvt.obj,gimagepsdraw.obj,gdrawgimage.obj,\
 gimagewriteeps.obj,\
 gimagexdraw.obj,gkeysym.obj,glist.obj,\
 gmenu.obj,gprogress.obj,gpsdraw.obj,gpstxtinit.obj,gradio.obj,gresource.obj,\
 gresourceimage.obj,gsavefiledlg.obj,gscrollbar.obj,gtabset.obj,\
 gtextfield.obj,gtextinfo.obj,gwidgets.obj,gxdraw.obj,ghvbox.obj,\
 gmatrixedit.obj,gspacer.obj,ctlvalues.obj,gcolor.obj,gresedit.obj,\
 xkeysyms_unicode.obj

libgdraw_OBJECTS1=gdrawable.obj,gxcdraw.obj

CFLAGS=/nowarn/incl=([-.inc])/name=(as_is,short)/define=("HAVE_CONFIG_H=1")

all : [-.libs]libgdraw.olb
	write sys$output "gdraw finished"

[-.libs]libgdraw.olb : $(libgdraw_OBJECTS) $(libgdraw_OBJECTS1)
	library/create [-.libs]libgdraw.olb $(libgdraw_OBJECTS)
	library [-.libs]libgdraw.olb $(libgdraw_OBJECTS1)

$(libgdraw_OBJECTS) : [-.inc]config.h
$(libgdraw_OBJECTS1) : [-.inc]config.h

choosericons.obj : choosericons.c
drawboxborder.obj : drawboxborder.c
gaskdlg.obj : gaskdlg.c
gbuttons.obj : gbuttons.c
gchardlg.obj : gchardlg.c
gcontainer.obj : gcontainer.c
gdraw.obj : gdraw.c
gdrawbuildchars.obj : gdrawbuildchars.c
gdrawerror.obj : gdrawerror.c
gdrawtxt.obj : gdrawtxt.c
gdrawtxtinit.obj : gdrawtxtinit.c
genkeysym.obj : genkeysym.c
gfilechooser.obj : gfilechooser.c
gfiledlg.obj : gfiledlg.c
ggadgets.obj : ggadgets.c
ggroupbox.obj : ggroupbox.c
gimageclut.obj : gimageclut.c
gimagecvt.obj : gimagecvt.c
gimagepsdraw.obj : gimagepsdraw.c
gimagewriteeps.obj : gimagewriteeps.c
gimagexdraw.obj : gimagexdraw.c
gkeysym.obj : gkeysym.c
glist.obj : glist.c
gmenu.obj : gmenu.c
gprogress.obj : gprogress.c
gpsdraw.obj : gpsdraw.c
gpstxtinit.obj : gpstxtinit.c
gradio.obj : gradio.c
gresource.obj : gresource.c
gresourceimage.obj : gresourceimage.c
gsavefiledlg.obj : gsavefiledlg.c
gscrollbar.obj : gscrollbar.c
gtabset.obj : gtabset.c
gtextfield.obj : gtextfield.c
gtextinfo.obj : gtextinfo.c
gwidgets.obj : gwidgets.c
gxdraw.obj : gxdraw.c
ghvbox.obj : ghvbox.c
gmatrixedit.obj : gmatrixedit.c
gspacer.obj : gspacer.c
gdrawable.obj : gdrawable.c
gdrawgimage.obj : gdrawgimage.c
ctlvalues.obj : ctlvalues.c
gcolor.obj : gcolor.c
xkeysyms_unicode.obj : xkeysyms_unicode.c
gxcdraw.obj : gxcdraw.c
gresedit.obj : gresedit.c
