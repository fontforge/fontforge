CFLAGS=/nowarn/incl=([-.inc])/name=(as_is,short)/define=("NODYNAMIC=1")


pfaedit_OBJECTS =  alignment.obj,autohint.obj,autosave.obj,autowidth.obj,\
 bitmapdlg.obj,metafont.obj,parsettfbmf.obj,\
 bitmapview.obj,bvedit.obj,charview.obj,charviewicons.obj,cursors.obj,\
 cvaddpoints.obj,cvexport.obj,cvgetinfo.obj,cvhints.obj,cvimages.obj,cvknife.obj,\
 cvpalettes.obj,cvpointer.obj,cvruler.obj,cvshapes.obj,cvstroke.obj,cvtranstools.obj,\
 cvundoes.obj,dumpbdf.obj,dumppfa.obj,fontinfo.obj,fontview.obj,fvcomposit.obj,\
 fvfonts.obj,fvimportbdf.obj,fvmetrics.obj,images.obj,metricsview.obj,\
 parsepfa.obj,parsettf.obj,prefs.obj,psread.obj,psunicodenames.obj,savefontdlg.obj,\
 sfd.obj,splashimage.obj,splinefill.obj,splineoverlap.obj,splinesave.obj,\
 splinesaveafm.obj,splinestroke.obj,splineutil.obj,splineutil2.obj,stamp.obj,\
 start.obj,tottf.obj,transform.obj,uiutil.obj,utils.obj,windowmenu.obj, \
 zapfnomen.obj,othersubrs.obj,autotrace.obj,openfontdlg.obj,encoding.obj,print.obj,\
 problems.obj,pfaedit-ui-en.obj,crctab.obj,macbinary.obj,scripting.obj
 
pfaedit_OBJECTS2=displayfonts.obj,combinations.obj,sftextfield.obj,ikarus.obj,\
        cvfreehand.obj,cvhand.obj,simplifydlg.obj,winfonts.obj,freetype.obj,\
	gotodlg.obj,search.obj,tottfgpos.obj,charinfo.obj,tottfaat.obj,\
	splineorder2.obj genttfinstrs.obj ttfinstrs.objs cvgridfit.obj

pfaedit.exe : nomen.h $(pfaedit_OBJECTS) $(pfaedit_OBJECTS2) xlib.opt
        library/create tmp.olb $(pfaedit_OBJECTS)
        library tmp.olb $(pfaedit_OBJECTS2)
        link/exec=pfaedit.exe start,tmp/lib,[-.libs]LIBGDRAW/lib,\
        LIBGUNICODE/lib,[]xlib.opt/opt


nomen.h : makenomenh.exe
        run makenomenh


makenomenh.exe : makenomenh.obj
        link makenomenh,[-.libs]LIBGDRAW/lib,LIBGUNICODE/lib,[]xlib.opt/opt


tottf.obj : tottf.c
          $(CC) $(CFLAGS)/noop tottf

tottfaat.obj : tottfaat.c
          $(CC) $(CFLAGS)/noop tottfaat
