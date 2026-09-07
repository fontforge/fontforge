# Distributed under the original FontForge BSD 3-clause license

configure_file(resources.in resources)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/resources"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/fontforge/pixmaps
  COMPONENT theme
)

install(FILES
  check_off_disabled.svg check_off.svg check_on_disabled.svg check_on.svg
  chooseraudio.svg chooserback.svg chooserbookmark.svg choosercid.svg choosercompressed.svg
  chooserconfigtool.svg choosercore.svg chooserdir.svg chooserforward.svg chooserhomefolder.svg
  chooserimage.svg choosermac.svg choosermacttf.svg choosernobookmark.svg chooserobject.svg
  choosersfdir.svg choosertar.svg choosertextbdf.svg choosertextc.svg choosertextcss.svg
  choosertextfontps.svg choosertextfontsfd.svg choosertexthtml.svg choosertextjava.svg
  choosertextmake.svg choosertextplain.svg choosertextps.svg choosertextxml.svg chooserttf.svg
  chooserunknown.svg chooserupdir.svg chooservideo.svg colorwheel.svg downarrow.svg downarrow_disabled.svg
  editclearback.svg editclear.svg editcopyfg2bg.svg editcopylayer2layer.svg editcopylbearing.svg
  editcopylookupdata.svg editcopy.svg editcopyrbearing.svg editcopyref.svg editcopyvwidth.svg
  editcopywidth.svg editcut.svg editfind.svg editjoin.svg editmerge.svg editmergetoline.svg
  editpasteafter.svg editpasteinto.svg editpaste.svg editredo.svg editrmundoes.svg editrplref.svg
  editsameas.svg editselect.svg editundo.svg editunlink.svg elementaddextrema.svg
  elementaddinflections.svg elementbalance.svg elementharmonize.svg elementalign.svg
  elementanticlock.svg elementautotrace.svg elementbdfinfo.svg elementbitmapsavail.svg
  elementbuildaccent.svg elementbuildcomposite.svg elementclockwise.svg elementcomparefonts.svg
  elementcomparelayers.svg elementcorrectdir.svg elementexpandstroke.svg elementfindprobs.svg
  elementfontinfo.svg elementgetinfo.svg elementglyphinfo.svg elementhbaselines.svg
  elementinterpolatefonts.svg elementmathinfo.svg elementmergefonts.svg elementorder.svg
  elementotherinfo.svg elementregenbitmaps.svg elementremovebitmaps.svg elementrenameglyph.svg
  elementround.svg elementshowdep.svg elementsimplify.svg elementstyles.svg
  elementtransform.svg elementvalidate.svg elementvbaselines.svg
  fflogo13.png fflogo.png ffsplash1.png ffsplash2.png ffsplash3.png
  fileclose2.svg fileclose.svg filedisplay.svg fileexecute.svg fileexport.svg filegeneratefamily.svg
  filegenerate.svg fileimport.svg filemergefeature.svg filenew.svg fileopen.svg fileprefs.svg
  fileprint.svg filequit.svg filerecent.svg filerevertbackup.svg filerevertglyph.svg filerevert.svg
  filesaveall.svg filesaveas.svg filesave.svg findinter.svg helpabout.svg helphelp.svg
  helpindex.svg hintsadddstem.svg hintsaddhstem.svg hintsaddvstem.svg hintsautohint.svg
  hintscleardstems.svg hintsclearhstems.svg hintsclearvstems.svg hintsdontautohint.svg
  hintsreviewhints.svg intersection.svg menuempty.svg metricscenter.svg
  metricssetlbearing.svg metricssetrbearing.svg metricssetvwidth.svg metricssetwidth.svg
  overlapexclude.svg overlapfindinter.svg overlapintersection.svg overlaprm.svg
  palette3drotate.svg palette3drotate-selected.svg palettecorner.svg palettecorner-selected.svg
  palettecurve.svg palettecurve-selected.svg paletteelipse.svg paletteelipse-selected.svg
  paletteflip.svg paletteflip-selected.svg palettefreehand.svg palettefreehand-selected.svg
  palettehand.svg palettehand-selected.svg palettehvcurve.svg palettehvcurve-selected.svg
  paletteknife.svg paletteknife-selected.svg paletteline.svg paletteline-selected.svg
  palettemagnify.svg palettemagnify-selected.svg palettepencil.svg palettepencil-selected.svg
  palettepen.svg palettepen-selected.svg paletteperspective.svg paletteperspective-selected.svg
  palettepointer.svg palettepointer-selected.svg palettepoly.svg palettepoly-selected.svg
  paletterect.svg paletterect-selected.svg paletterotate.svg paletterotate-selected.svg
  paletteruler.svg paletteruler-selected.svg palettescale.svg palettescale-selected.svg paletteselectedbg.svg
  paletteshift.svg paletteshift-selected.svg paletteskew.svg paletteskew-selected.svg
  palettesmall3drotate.svg palettesmallcorner.svg palettesmallcurve.svg palettesmallelipse.svg
  palettesmallflip.svg palettesmallhand.svg palettesmallhvcurve.svg palettesmallknife.svg
  palettesmallmag.svg palettesmallpencil.svg palettesmallpen.svg palettesmallperspective.svg
  palettesmallpointer.svg palettesmallpoly.svg palettesmallrect.svg palettesmallrotate.svg
  palettesmallruler.svg palettesmallscale.svg palettesmallskew.svg palettesmallspirocorner.svg
  palettesmallspirocurve.svg palettesmallspirog2curve.svg palettesmallspiroleft.svg
  palettesmallspiroright.svg palettesmallstar.svg palettesmalltangent.svg palettespirocorner.svg
  palettespirocorner-selected.svg palettespirocurve.svg palettespirocurve-selected.svg
  palettespirodisabled.svg palettespirodown.svg palettespirog2curve.svg
  palettespirog2curve-selected.svg palettespiroleft.svg palettespiroleft-selected.svg
  palettespiroright.svg palettespiroright-selected.svg palettespiroup.svg
  palettespiroup-selected.svg palettestar.svg palettestar-selected.svg palettetangent.svg
  palettetangent-selected.svg pointsaddanchor.svg pointscorner.svg pointscurve.svg
  pointsG2curve.svg pointshvcurve.svg pointsmakearc.svg pointsmakeline.svg pointsnamecontour.svg
  pointsnamepoint.svg pointsspironext.svg pointsspiroprev.svg pointstangent.svg ptinfocorner.svg
  ptinfocurve.svg ptinfohvcurve.svg ptinfotangent.svg python.svg radio_off_disabled.svg radio_off.svg
  radio_on_disabled.svg radio_on.svg rmoverlap.svg
  selectblue.svg selectcyan.svg selectdefault.svg selectgreen.svg selectmagenta.svg selectred.svg
  selectwhite.svg selectyellow.svg styleschangeweight.svg styleschangexheight.svg
  stylesextendcondense.svg stylesinline.svg stylesitalic.svg stylesoblique.svg stylesoutline.svg
  stylesshadow.svg stylessmallcaps.svg stylessubsuper.svg styleswireframe.svg
  tools3drotate.svg toolselipse.svg toolsflip.svg toolsfreehand.svg toolsknife.svg toolsmagnify.svg
  toolspen.svg toolsperspective.svg toolspointer.svg toolspolygon.svg toolsrect.svg toolsrotate.svg
  toolsruler.svg toolsscale.svg toolsscroll.svg toolsskew.svg toolsspiro.svg toolsstar.svg
  transformfliphor.svg transformflipvert.svg transformrotate180.svg transformrotateccw.svg
  transformrotatecw.svg transformskew.svg ttdebugcontinue.svg ttdebugexit.svg ttdebugmenudelta.svg
  ttdebugstepinto.svg ttdebugstepout.svg ttdebugstepover.svg ttdebugstopped.svg ttdebugstop.svg
  ttdebugwatchpnt.svg viewbiggersize.svg viewfindinfont.svg viewfit.svg viewformer.svg viewgoto.svg
  viewinsertafter.svg viewinsertbefore.svg viewlayers.svg viewnextdef.svg viewnext.svg
  viewpalettes.svg viewprevdef.svg viewprev.svg viewreplace.svg viewsmallersize.svg viewzoomin.svg
  viewzoomout.svg
  visibility_off_disabled.svg visibility_off.svg visibility_on_disabled.svg visibility_on.svg
  changeweight.svg exclude.svg extendcondense.svg fliphor.svg flipvert.svg inline.svg oblique.svg 
  outline.svg rotate180.svg rotateccw.svg rotatecw.svg shadow.svg skew.svg text12210.svg wireframe.svg

  charviewicon.xbm fontview2.xbm logo.xbm

  splash2019.png splash2020.png

  DESTINATION ${CMAKE_INSTALL_DATADIR}/fontforge/pixmaps
  COMPONENT theme
)
