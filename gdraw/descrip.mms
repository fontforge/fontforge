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
 gtextfield.obj,gtextinfo.obj,gwidgets.obj,gxdraw.obj

CFLAGS=/nowarn/incl=([-.inc])/name=(as_is,short)/define=("NOTHREADS=1")

all : [-.libs]libgdraw.olb
	write sys$output "gdraw finished"

[-.libs]libgdraw.olb : $(libgdraw_OBJECTS)
	library/create [-.libs]libgdraw.olb $(libgdraw_OBJECTS)
