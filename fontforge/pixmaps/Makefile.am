# Copyright (C) 2000-2012 by George Williams
# Copyright (C) 2012 by Barry Schwartz
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# The name of the author may not be used to endorse or promote products
# derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

include $(top_srcdir)/mk/layout.am

PNG_LIST = chooseraudio.png chooserback.png chooserbookmark.png			\
	choosercid.png choosercompressed.png chooserconfigtool.png			\
	choosercore.png chooserdir.png chooserforward.png					\
	chooserhomefolder.png chooserimage.png choosermac.png				\
	choosermacttf.png choosernobookmark.png chooserobject.png			\
	choosertar.png choosertextbdf.png choosertextc.png					\
	choosertextcss.png choosertextfontps.png choosertextfontsfd.png		\
	choosertexthtml.png choosertextjava.png choosertextmake.png			\
	choosertextplain.png choosertextps.png choosertextxml.png			\
	chooserttf.png chooserunknown.png chooserupdir.png					\
	chooservideo.png colorwheel.png editclearback.png editclear.png		\
	editcopyfg2bg.png editcopylayer2layer.png editcopylbearing.png		\
	editcopylookupdata.png editcopy.png editcopyrbearing.png			\
	editcopyref.png editcopyvwidth.png editcopywidth.png editcut.png	\
	editfind.png editjoin.png editmerge.png editpasteafter.png			\
	editpasteinto.png editpaste.png editredo.png editrmundoes.png		\
	editrplref.png editsameas.png editselect.png editundo.png			\
	editunlink.png elementaddextrema.png elementalign.png				\
	elementanticlock.png elementautotrace.png elementbdfinfo.png		\
	elementbitmapsavail.png elementbuildaccent.png						\
	elementbuildcomposite.png elementclockwise.png						\
	elementcomparefonts.png elementcomparelayers.png					\
	elementcorrectdir.png elementexpandstroke.png						\
	elementfindprobs.png elementfontinfo.png elementgetinfo.png			\
	elementglyphinfo.png elementhbaselines.png							\
	elementinterpolatefonts.png elementmathinfo.png						\
	elementmergefonts.png elementorder.png elementotherinfo.png			\
	elementregenbitmaps.png elementremovebitmaps.png					\
	elementrenameglyph.png elementround.png elementshowdep.png			\
	elementsimplify.png elementstyles.png elementtilepath.png			\
	elementtilepattern.png elementtransform.png elementvalidate.png		\
	elementvbaselines.png fileclose.png fileexecute.png					\
	fileexport.png filegeneratefamily.png filegenerate.png				\
	fileimport.png filemergefeature.png filenew.png fileopen.png		\
	fileprefs.png fileprint.png filequit.png filerecent.png				\
	filerevertbackup.png filerevertglyph.png filerevert.png				\
	filesaveall.png filesaveas.png filesave.png helpabout.png			\
	helphelp.png helpindex.png hintsadddstem.png hintsaddhstem.png		\
	hintsaddvstem.png hintsautohint.png hintscleardstems.png			\
	hintsclearhstems.png hintsclearvstems.png hintsdontautohint.png		\
	hintsreviewhints.png menuempty.png metricscenter.png				\
	metricssetlbearing.png metricssetrbearing.png						\
	metricssetvwidth.png metricssetwidth.png overlapexclude.png			\
	overlapfindinter.png overlapintersection.png overlaprm.png			\
	palette3drotate.png palettecorner.png palettecurve.png				\
	paletteelipse.png paletteflip.png palettefreehand.png				\
	palettehand.png palettehvcurve.png paletteknife.png					\
	paletteline.png palettemagnify.png palettepencil.png				\
	palettepen.png paletteperspective.png palettepointer.png			\
	palettepoly.png paletterect.png paletterotate.png					\
	paletteruler.png palettescale.png paletteshift.png					\
	paletteskew.png palettesmall3drotate.png palettesmallcorner.png		\
	palettesmallcurve.png palettesmallelipse.png palettesmallflip.png	\
	palettesmallhand.png palettesmallhvcurve.png						\
	palettesmallknife.png palettesmallmag.png palettesmallpencil.png	\
	palettesmallpen.png palettesmallperspective.png						\
	palettesmallpointer.png palettesmallpoly.png palettesmallrect.png	\
	palettesmallrotate.png palettesmallruler.png						\
	palettesmallscale.png palettesmallskew.png							\
	palettesmallspirocorner.png palettesmallspirocurve.png				\
	palettesmallspirog2curve.png palettesmallspiroleft.png				\
	palettesmallspiroright.png palettesmallstar.png						\
	palettesmalltangent.png palettespirocorner.png						\
	palettespirocurve.png palettespirodisabled.png						\
	palettespirodown.png palettespirog2curve.png palettespiroleft.png	\
	palettespiroright.png palettespiroup.png palettestar.png			\
	palettetangent.png pointsaddanchor.png pointscorner.png				\
	pointscurve.png pointsG2curve.png pointshvcurve.png					\
	pointsmakearc.png pointsmakeline.png pointsnamecontour.png			\
	pointsspironext.png pointsspiroprev.png pointstangent.png			\
	ptinfocorner.png ptinfocurve.png ptinfohvcurve.png					\
	ptinfotangent.png python.png styleschangeweight.png					\
	styleschangexheight.png stylesextendcondense.png stylesinline.png	\
	stylesitalic.png stylesoblique.png stylesoutline.png				\
	stylesshadow.png stylessmallcaps.png stylessubsuper.png				\
	styleswireframe.png tools3drotate.png toolselipse.png				\
	toolsflip.png toolsfreehand.png toolsknife.png toolsmagnify.png		\
	toolspen.png toolsperspective.png toolspointer.png					\
	toolspolygon.png toolsrect.png toolsrotate.png toolsruler.png		\
	toolsscale.png toolsscroll.png toolsskew.png toolsspiro.png			\
	toolsstar.png transformfliphor.png transformflipvert.png			\
	transformrotate180.png transformrotateccw.png						\
	transformrotatecw.png transformskew.png ttdebugcontinue.png			\
	ttdebugexit.png ttdebugmenudelta.png ttdebugstepinto.png			\
	ttdebugstepout.png ttdebugstepover.png ttdebugstopped.png			\
	ttdebugstop.png ttdebugwatchpnt.png viewbiggersize.png				\
	viewfindinfont.png viewfit.png viewformer.png viewgoto.png			\
	viewinsertafter.png viewinsertbefore.png viewlayers.png				\
	viewnextdef.png viewnext.png viewpalettes.png viewprevdef.png		\
	viewprev.png viewreplace.png viewsmallersize.png viewzoomin.png		\
	viewzoomout.png

dist_pixmaps_DATA = $(PNG_LIST)

-include $(top_srcdir)/git.mk
