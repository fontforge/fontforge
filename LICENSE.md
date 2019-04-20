# FontForge Licensing

FontForge is available as a whole under the terms of the [GNU GPL](http://www.gnu.org/copyleft/gpl.html), version
3 or any later version. 
However, almost all of its parts are available under the "revised BSD license" ([pdf](http://www.law.yi.org/~sfllaw/talks/bsd.pdf)) because FontForge was mostly written by George Williams, using that license.

The Revised BSD License is very permissive, and allows for code to be combined with other code under other licenses. 
There are many useful libraries available under copyleft libre licenses, such as the LGPL and GPL, which FontForge started to use in 2012. 
For example, Pango and Cairo are available under the LGPL.
Some features added since 2012 are licensed by their individual developers under the GPLv3.

## How To License Contributions

- Contributions to existing files must be made under the existing license for that file
- Contributions in new files should be made under the original Revised BSD License, but any license compatible with the GPLv3 is fine

The majority of FontForge was written by George Williams and published under the revised BSD license. 
This license is permitted to be compiled with added GNU GPL source code to build a FontForge executable program.

FontForge was known from 2000 to 2004 as PfaEdit, and was written and developed primarily by George Williams from 2000 until 2012.
It is published as free/libre software and distributed under the 3-clause BSD license. 
Later, when development slowed down and then stopped in 2012, the FontForge community began to take an interest in improving FontForge further with fixes, modifications, libraries, patches, and other additions which have been introduced under the same, or different, yet, compatible licenses.

## Copyright Notices, Contributors, Translators

See [`AUTHORS`](https://github.com/fontforge/fontforge/blob/master/AUTHORS)

## The Revised BSD License used by FontForge

```
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

The name of the author may not be used to endorse or promote
products derived from this software without specific prior written
permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

## LGPL and GPL

FontForge includes the GPLv3 in the source tree as [COPYING.gplv3](https://github.com/fontforge/fontforge/blob/master/COPYING.gplv3)

You can read the [LGPL](http://www.gnu.org/licenses/lgpl.html) and [GPL](http://www.gnu.org/copyleft/gpl.html) on the GNU website.

## Source Files

### FontForge Files Using The Revised BSD License

These files were created under the Revised BSD License:
```
fontforge-20120731-b/
install-sh

fontforge-20120731-b/fontforge:
alignment.c		fontinfo.c		savefont.h
anchorsaway.c		fontview.c		savefontdlg.c
asmfpst.c		fontviewbase.c		scriptfuncs.h
autohint.c		freetype.c		scripting.c
autosave.c		freetypeui.c		scripting.h
autotrace.c		ftdelta.c		scriptingdlg.c
autowidth.c		fvcomposite.c		scstyles.c
autowidth2.c		fvfonts.c		scstylesui.c
autowidth2.h		fvfontsdlg.c		sd.h
autowidth2dlg.c		fvmetrics.c		search.c
basedlg.c		fvmetrics.h		search.h
baseviews.h		fvmetricsdlg.c		searchview.c
bdfinfo.c		glyphcomp.c		sfd.c
bitmapchar.c		gotodlg.c		sfd1.c
bitmapcontrol.c		groups.c		sfd1.h
bitmapcontrol.h		groups.h		sfddiff.c
bitmapdlg.c		groupsdlg.c		sflayout.c
bitmapview.c		histograms.c		sflayoutP.h
bvedit.c		http.c			sftextfield.c
charinfo.c		ikarus.c		sftextfieldP.h
charview.c		images.c		showatt.c
clipnoui.c		justifydlg.c		simplifydlg.c
clipui.c		kernclass.c		spiro.c
combinations.c		layer2layer.c		splashimage.c
configure-fontforge.h	lookups.c		splinechar.c
contextchain.c		lookupui.c		splinefill.c
cursors.c		macbinary.c		splinefont.c
cvaddpoints.c		macenc.c		splinefont.h
cvdebug.c		macencui.c		splineorder2.c
cvdgloss.c		math.c			splineoverlap.c
cvexport.c		mathconstants.c		splinerefigure.c
cvexportdlg.c		metricsview.c		splinesaveafm.c
cvfreehand.c		mm.c			splinesave.c
cvgetinfo.c		mm.h			splinestroke.c
cvgridfit.c		mmdlg.c			splineutil.c
cvhand.c		namehash.h		splineutil2.c
cvhints.c		namelist.c		start.c
cvimages.c		nonlineartrans.c	startnoui.c
cvimportdlg.c		nonlineartrans.h	startui.c
cvknife.c		nonlineartransui.c	statemachine.c
cvpalettes.c		noprefs.c		stemdb.c
cvpointer.c		nouiutil.c		stemdb.h
cvruler.c		nowakowskittfinstr.c	svg.c
cvshapes.c		tilepath.c
cvstroke.c		openfontdlg.c		tottf.c
cvtranstools.c		palmfonts.c		tottfaat.c
cvundoes.c		parsepdf.c		tottfgpos.c
delta.h			parsepfa.c		tottfvar.c
deltaui.c		parsettf.c		transform.c
displayfonts.c		parsettfatt.c		ttf.h
dumpbdf.c		parsettfbmf.c		ttfinstrs.c
dumppfa.c		parsettfvar.c		ttfinstrs.h
edgelist.h		PfEd.h			ttfinstrsui.c
edgelist2.h		ttfspecial.c
effects.c		ufo.c
effectsui.c		prefs.c			uiinterface.h
encoding.c		print.c			uiutil.c
encodingui.c		print.h			unicoderange.c
featurefile.c		problems.c		unicoderange.h
fffreetype.h		psfont.h		utils.c
ffpython.h		psread.c		views.h
fontforge.h		python.c		windowmenu.c
fontforgeui.h		pythonui.c		winfonts.c
fontforgevw.h		savefont.c		woff.c

fontforge-20120731-b/gdraw:
choosericons.c		gfiledlg.c		gradio.c
colorP.h		ggadgetP.h		gresedit.c
ctlvalues.c		ggadgets.c		gresource.c
drawboxborder.c		ggroupbox.c		gresourceimage.c
gaskdlg.c		ghvbox.c		gresourceP.h
gbuttons.c		gimagebmpP.h		growcol.c
gchardlg.c		gimageclut.c		gsavefiledlg.c
gcolor.c		gimagecvt.c		gscrollbar.c
gcontainer.c		gimagepsdraw.c		gspacer.c
gdraw.c			gimagewriteeps.c	gtabset.c
gdrawable.c		gimagexdraw.c		gtextfield.c
gdrawbuildchars.c	gkeysym.c		gtextinfo.c
gdrawerror.c		glist.c			gwidgetP.h
gdrawgimage.c		gmatrixedit.c		gwidgets.c
gdrawP.h		gmenu.c			gxcdraw.c
gdrawtxt.c		gprogress.c		gxcdrawP.h
gdrawtxtinit.c		gpsdraw.c		gxdrawP.h
gdrawwacomdriver.c	gpsdrawP.h		hotkeys.c
gfilechooser.c		gpstxtinit.c		hotkeys.h

fontforge-20120731-b/gutils:
dynamic.c		gimagereadrgb.c		giofile.c
fsys.c			gimagereadtiff.c	gioftp.c
gcol.c			gimagereadxbm.c		gioftpP.h
gimage.c		gimagereadxpm.c		giofuncP.h
gimagebmpP.h		gimagewritebmp.c	giohosts.c
gimageread.c		gimagewritegimage.c	giomime.c
gimagereadbmp.c		gimagewritejpeg.c	gioP.h
gimagereadgif.c		gimagewritepng.c	giothread.c
gimagereadjpeg.c	gimagewritexbm.c	giotrans.c
gimagereadpng.c		gimagewritexpm.c	gwwintl.c
gimagereadras.c		gio.c

fontforge-20120731-b/inc:
basics.h	gfile.h		gio.h		gwidget.h
charset.h	ggadget.h	gprogress.h	gwwiconv.h
fileutil.h	gicons.h	gresedit.h	intl.h
gdraw.h		gimage.h	gresource.h	ustring.h

fontforge-20120731-b/Packaging:
fontforge.desktop	FontForge.spec		fontforge.xml
FontForge-doc.spec	FontForge.static.spec

fontforge-20120731-b/pycontrib:
excepthook.py

fontforge-20120731-b/Unicode:
char.c		makebuildtables.c	ucharmap.c
dump.c		makeutype.c
gwwiconv.c	memory.c		ustring.c
```

### FontForge Files Using The GPL Licenses

These files were created under the GPL License:
```
fontforge-20120731-b/
aclocal.m4	configure.dynamic	configure.static.in
config.guess	configure.dynamic.in	ltmain.sh
config.sub	configure.in		Makefile.dynamic.in
configure	configure.static	Makefile.static.in

fontforge-20120731-b/fontforge:
Makefile.dynamic.in	Makefile.static.in

fontforge-20120731-b/gdraw:
Makefile.dynamic.in	Makefile.static.in

fontforge-20120731-b/gutils:
giomime.c	Makefile.dynamic.in	Makefile.static.in

fontforge-20120731-b/inc:
fontforge-config.h.in

fontforge-20120731-b/plugins:
ANALYZE_MAP.COM	Makefile.in

fontforge-20120731-b/Unicode:
Makefile.dynamic.in	Makefile.static.in
```

### FontForge Files Using The Open Group License

```
fontforge-20120731-b/fontforge:
fvimportbdf.c

fontforge-20120731-b/gdraw:
fontP.h

fontforge-20120731-b/inc:
gkeysym.h

### Other Files

These are generated files, or do not have a license specified:

```
fontforge-20120731-b/
LICENSE		README-Unix.html
fontforge.pc.in	README-Mac.html	systemspecific.in
INSTALL		README-unix	VERSION

fontforge-20120731-b/fontforge:
acorn2sfd.c		fontimage.pe		pottitle.patch
autowidth.h		fontlint.1		pua.c
bezctx_ff.c		fontlint.pe		sfddiff.1
bezctx_ff.h		langfreq.c		sfddiff.pe
crctab.c		libffstamp.h		stamp.c
libstamp.pre		stamper.c
diffstubs.c		threaddummy.c
encoding.h		lookups.h		utf8.pot
exelibstamp.pre		MacFontForgeApp.zip
fontforge.1		othersubrs.c		zapfnomen.c
fontimage.1		potmstitle.patch

fontforge-20120731-b/gdraw:
genkeysym.c	gxdraw.c	xkeysyms_unicode.c

fontforge-20120731-b/inc:
chardata.h	dynamic.h	utype.h

fontforge-20120731-b/gutils:
divisors.c

fontforge-20120731-b/plugins:
gb12345.c

fontforge-20120731-b/po:
ca.po  el.po     es.po  it.po  ml.po  ru.po     uk.po  zh_CN.po
de.po  en_GB.po  fr.po  ja.po  pl.po  toengb.c  vi.po  zh_TW.po

fontforge-20120731-b/pyhook:
fontforgepyhook.c	psMatpyhook.c	setup.py.in
loadfontforge.h		README

fontforge-20120731-b/Unicode:
alphabet.c	cjk.c		README.TXT
ArabicForms.c	combiners.h	unialt.c
backtrns.c	utype.c
```
