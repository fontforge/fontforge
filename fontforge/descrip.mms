CFLAGS=/nowarn/incl=([-.inc])/name=(as_is,short)/define=("NODYNAMIC=1",\
        "FONTFORGE_CONFIG_DEVICETABLES=1")

fontforge_OBJECTS =  alignment.obj,autohint.obj,autosave.obj,autowidth.obj,\
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
 problems.obj,crctab.obj,macbinary.obj,scripting.obj
 
fontforge_OBJECTS2=displayfonts.obj,combinations.obj,sftextfield.obj,ikarus.obj,\
        cvfreehand.obj,cvhand.obj,simplifydlg.obj,winfonts.obj,freetype.obj,\
	gotodlg.obj,search.obj,tottfgpos.obj,charinfo.obj,tottfaat.obj,\
	splineorder2.obj,genttfinstrs.obj,ttfinstrs.obj,cvgridfit.obj,\
	cvdebug.obj,showatt.obj,kernclass.obj,nonlineartrans.obj,effects.obj,\
	histograms.obj,ttfspecial.obj,svg.obj,parsettfatt.obj,contextchain.obj,\
	macenc.obj,statemachine.obj,typofeatures.obj,splinerefigure.obj,mm.obj,\
	parsettfvar.obj,tottfvar.obj,pua.obj,stemdb.obj,anchorsaway.obj,\
	palmfonts.obj,cvdgloss.obj,groups.obj,parsepdf.obj

fontforge.exe : $(fontforge_OBJECTS) $(fontforge_OBJECTS2) xlib.opt
        library/create tmp.olb $(fontforge_OBJECTS)
        library tmp.olb $(fontforge_OBJECTS2)
        link/exec=fontforge.exe start,tmp/lib,[-.libs]LIBGDRAW/lib,\
        LIBGUNICODE/lib,[]xlib.opt/opt

alignment.obj : alignment.c
autohint.obj : autohint.c
autosave.obj : autosave.c
autowidth.obj : autowidth.c
bitmapdlg.obj : bitmapdlg.c
metafont.obj : metafont.c
parsettfbmf.obj : parsettfbmf.c
bitmapview.obj : bitmapview.c
bvedit.obj : bvedit.c
charview.obj : charview.c
charviewicons.obj : charviewicons.c
cursors.obj : cursors.c
cvaddpoints.obj : cvaddpoints.c
cvexport.obj : cvexport.c
cvgetinfo.obj : cvgetinfo.c
cvhints.obj : cvhints.c
cvimages.obj : cvimages.c
cvknife.obj : cvknife.c
cvpalettes.obj : cvpalettes.c
cvpointer.obj : cvpointer.c
cvruler.obj : cvruler.c
cvshapes.obj : cvshapes.c
cvstroke.obj : cvstroke.c
cvtranstools. : cvtranstools.c
cvundoes.obj : cvundoes.c
dumpbdf.obj : dumpbdf.c
dumppfa.obj : dumppfa.c
fontinfo.obj : fontinfo.c
fontview.obj : fontview.c
fvcomposit.obj : fvcomposit.c
fvfonts.obj : fvfonts.c
fvimportbdf.obj : fvimportbdf.c
fvmetrics.obj : fvmetrics.c
images.obj : images.c
metricsview.obj : metricsview.c
parsepfa.obj : parsepfa.c
parsettf.obj : parsettf.c
prefs.obj : prefs.c
psread.obj : psread.c
psunicodenames.obj : psunicodenames.c
savefontdlg.ob : savefontdlg.c
sfd.obj : sfd.c
splashimage.obj : splashimage.c
splinefill.obj : splinefill.c
splineoverlap.obj : splineoverlap.c
splinesave.obj : splinesave.c
splinesaveafm.obj : splinesaveafm.c
splinestroke.obj : splinestroke.c
splineutil.obj : splineutil.c
splineutil2.obj : splineutil2.c
stamp.obj : stamp.c
start.obj : start.c
tottf.obj : tottf.c
          $(CC) $(CFLAGS)/noop tottf
transform.obj : transform.c
uiutil.obj : uiutil.c
utils.obj : utils.c
windowmenu.obj : windowmenu.c
zapfnomen.obj : zapfnomen.c
othersubrs.obj : othersubrs.c
autotrace.obj : autotrace.c
openfontdlg.obj : openfontdlg.c
encoding.obj : encoding.c
print.ob : print.c
problems.obj : problems.c
crctab.obj : crctab.c
macbinary.obj : macbinary.c
scripting.obj : scripting.c
displayfonts.obj : displayfonts.c
combinations.obj : combinations.c
sftextfield.obj : sftextfield.c
ikarus.obj : ikarus.c
cvfreehand.obj : cvfreehand.c
cvhand.obj : cvhand.c
simplifydlg.obj : simplifydlg.c
winfonts.obj : winfonts.c
freetype.obj : freetype.c
gotodlg.obj : gotodlg.c
search.obj : search.c
tottfgpos.obj : tottfgpos.c
charinfo.obj : charinfo.c
tottfaat.obj : tottfaat.c
          $(CC) $(CFLAGS)/noop tottfaat
splineorder2.obj : splineorder2.c
genttfinstrs.obj : genttfinstrs.c
ttfinstrs.obj : ttfinstrs.c
cvgridfit.obj : cvgridfit.c
cvdebug.obj : cvdebug.c
showatt.obj : showatt.c
kernclass.obj : kernclass.c
nonlineartrans.obj : nonlineartrans.c
effects.obj : effects.c
histograms.obj : histograms.c
ttfspecial.obj : ttfspecial.c
svg.obj : svg.c
parsettfatt.obj : parsettfatt.c
contextchain.obj : contextchain.c
macenc.obj : macenc.c
statemachine.obj : statemachine.c
typofeatures.obj : typofeatures.c
splinerefigure.obj : splinerefigure.c
mm.obj : mm.c
parsettfvar.obj : parsettfvar.c
tottfvar.obj : tottfvar.c
pua.obj : pua.c
stemdb.obj : stemdb.c
anchorsaway.obj : anchorsaway.c
palmfonts.obj : palmfonts.c
cvdgloss.obj : cvdgloss.c
groups.obj : groups.c
parsepdf.obj : parsepdf.c
