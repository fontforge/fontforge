# Makefile for OpenVMS
# Date : 28 April 2008

libgutil_OBJECTS =divisors.obj,dynamic.obj,fsys.obj,gimage.obj,gimageread.obj\
 ,gimagereadbmp.obj,gimagereadgif.obj,gimagereadjpeg.obj,gimagereadpng.obj,\
 gimagereadras.obj,gimagereadrgb.obj,gimagereadtiff.obj,gimagereadxbm.obj,\
 gimagereadxpm.obj,gimagewritebmp.obj,gimagewritegimage.obj,\
 gimagewritejpeg.obj,gimagewritepng.obj,\
 gimagewritexbm.obj,gwwintl.obj,gio.obj,giofile.obj,gioftp.obj,giohosts.obj,\
 giomime.obj,giothread.obj,giotrans.obj,gcol.obj

CFLAGS=/nowarn/incl=([-.inc])/name=(as_is,short)\
	/define=("NOTHREADS=1","_NO_XKB=1","_STATIC_LIBFREETYPE=1",\
	"_STATIC_LIBPNG=1","HAVE_LIBINTL_H=1","_NO_XINPUT=1",\
	"_STATIC_LIBUNINAMESLIST=1","_STATIC_LIBXML=1",\
	"_STATIC_LIBUNGIF=1","_STATIC_LIBJPEG=1","_STATIC_LIBTIFF=1",\
	"HAVE_PTHREAD_H=1")

all : [-.libs]libgutil.olb
	write sys$output "gutil finished"

[-.libs]libgutil.olb : $(libgutil_OBJECTS) $(libgutil_OBJECTS1)
	library/create [-.libs]libgutil.olb $(libgutil_OBJECTS)

divisors.obj : divisors.c
dynamic.obj : dynamic.c
fsys.obj : fsys.c
gimage.obj : gimage.c
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
gimagewritegimage.obj : gimagewritegimage.c
gimagewritejpeg.obj : gimagewritejpeg.c
gimagewritepng.obj : gimagewritepng.c
gimagewritexbm.obj : gimagewritexbm.c
gimagewritexpm.obj : gimagewritexpm.c
gwwintl.obj : gwwintl.c
gio.obj : gio.c
giofile.obj : giofile.c
gioftp.obj : gioftp.c
giohosts.obj : giohosts.c
giomime.obj : giomime.c
giothread.obj : giothread.c
giotrans.obj : giotrans.c
gcol.obj : gcol.c
