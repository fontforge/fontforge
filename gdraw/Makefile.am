GD_VERSION = 4
GD_REVISION = 10
GD_AGE = 0

lib_LTLIBRARIES = libgdraw.la

libgdraw_la_SOURCES = choosericons.c ctlvalues.c drawboxborder.c		\
	gaskdlg.c gbuttons.c gcolor.c gchardlg.c gcontainer.c gdraw.c		\
	gdrawbuildchars.c gdrawerror.c gdrawtxt.c gdrawtxtinit.c			\
	gfilechooser.c gfiledlg.c ggadgets.c ggroupbox.c gimageclut.c		\
	gimagecvt.c gimagepsdraw.c gimagewriteeps.c gdrawgimage.c			\
	gimagexdraw.c gkeysym.c glist.c gmenu.c gprogress.c gpsdraw.c		\
	gpstxtinit.c gradio.c gresource.c gresourceimage.c gresedit.c		\
	gsavefiledlg.c gscrollbar.c gtabset.c gtextfield.c gtextinfo.c		\
	gwidgets.c gxdraw.c gxcdraw.c ghvbox.c gmatrixedit.c gdrawable.c	\
	gspacer.c xkeysyms_unicode.c colorP.h gdrawP.h gimagebmpP.h			\
	gresourceP.h gxcdrawP.h fontP.h ggadgetP.h gpsdrawP.h gwidgetP.h	\
	gxdrawP.h

libgdraw_la_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"	\
	$(MY_CFLAGS)

libgdraw_la_LIBADD = $(top_builddir)/Unicode/libgunicode.la	\
	$(top_builddir)/gutils/libgutils.la

libgdraw_la_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -version-info	\
	"$(GD_VERSION):$(GD_REVISION):$(GD_AGE)"
