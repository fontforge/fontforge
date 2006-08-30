libgdraw_OBJECTS =  choosericons.obj,divisors.obj,drawboxborder.obj,\
 fsys.obj,gaskdlg.obj,gbuttons.obj,gchardlg.obj,gcontainer.obj,gdraw.obj,\
 gdrawbuildchars.obj,gdrawerror.obj,gdrawtxt.obj,gdrawtxtinit.obj,\
 genkeysym.obj,gfilechooser.obj,gfiledlg.obj,ggadgets.obj,ggroupbox.obj,\
 gimage.obj,gimageclut.obj,gimagecvt.obj,gimagepsdraw.obj,gimageread.obj,\
 gimagereadbmp.obj,gimagereadgif.obj,gimagereadjpeg.obj,gimagereadpng.obj,\
 gimagereadras.obj,gimagereadrgb.obj,gimagereadtiff.obj,gimagereadxbm.obj,\
 gimagereadxpm.obj,gimagewritebmp.obj,gimagewriteeps.obj,\
 gimagewritegimage.obj,gimagewritejpeg.obj,gimagewritepng.obj,\
 gimagewritexbm.obj,gimagewritexpm.obj,gimagexdraw.obj,gio.obj,giofile.obj,\
 giohosts.obj,giomime.obj,giothread.obj,giotrans.obj,gkeysym.obj,glist.obj,\
 gmenu.obj,gprogress.obj,gpsdraw.obj,gpstxtinit.obj,gradio.obj,gresource.obj,\
 gresourceimage.obj,gsavefiledlg.obj,gscrollbar.obj,gtabset.obj,\
 gtextfield.obj,gtextinfo.obj,gwidgets.obj,gxdraw.obj,ghvbox.obj,\
 gmatrixedit.obj

CFLAGS=/nowarn/incl=([-.inc])/name=(as_is,short)\
	/define=("NOTHREADS=1","_NO_XKB=1","_STATIC_LIBFREETYPE=1",\
	"_STATIC_LIBPNG=1","HAVE_LIBINTL_H=1",\
	"_STATIC_LIBUNINAMESLIST=1","_STATIC_LIBXML=1",\
	"_STATIC_LIBUNGIF=1","_STATIC_LIBJPEG=1","_STATIC_LIBTIFF=1")

all : [-.libs]libgdraw.olb
	write sys$output "gdraw finished"

[-.libs]libgdraw.olb : $(libgdraw_OBJECTS)
	library/create [-.libs]libgdraw.olb $(libgdraw_OBJECTS)

choosericons.obj : choosericons.c
divisors.obj : divisors.c
drawboxborder.obj : drawboxborder.c
fsys.obj : fsys.c
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
gimage.obj : gimage.c
gimageclut.obj : gimageclut.c
gimagecvt.obj : gimagecvt.c
gimagepsdraw.obj : gimagepsdraw.c
gimageread.obj : gimageread.c
gimagereadbmp.obj : gimagereadbmp.c
gimagereadgif.obj : gimagereadgif.c
gimagereadjpeg.obj : gimagereadjpeg.c
gimagereadpng.obj : gimagereadpng.c
gimagereadras.obj : gimagereadras.c
gimagereadrgb.obj : gimagereadrgb.c
gimagereadtiff.obj : gimagereadtiff.c
gimagereadxbm.obj : gimagereadxbm.c
gimagereadxpm.obj : gimagereadxpm.c
gimagewritebmp.obj : gimagewritebmp.c
gimagewriteeps.obj : gimagewriteeps.c
gimagewritegimage.obj : gimagewritegimage.c
gimagewritejpeg.obj : gimagewritejpeg.c
gimagewritepng.obj : gimagewritepng.c
gimagewritexbm.obj : gimagewritexbm.c
gimagewritexpm.obj : gimagewritexpm.c
gimagexdraw.obj : gimagexdraw.c
gio.obj : gio.c
giofile.obj : giofile.c
giohosts.obj : giohosts.c
giomime.obj : giomime.c
giothread.obj : giothread.c
giotrans.obj : giotrans.c
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
