# Distributed under the original FontForge BSD 3-clause license

# not included
#  gdrawwacomdriver.c genkeysym.c growcol.c

add_library(gdraw STATIC
  choosericons.c
  ghelp.c
  colorP.h
  ctlvalues.c
  div_tables.c
  drawboxborder.c
  fontP.h
  gaskdlg.c
  gbuttons.c
  gcolor.c
  gcontainer.c
  gdrawable.c
  gdraw.c
  gdrawerror.c
  gdrawgimage.c
  gdrawP.h
  gdrawtxt.c
  gfilechooser.c
  gfiledlg.c
  gflowbox.c
  ggadgetP.h
  ggadgets.c
  ggdkcdraw.c
  ggdkdraw.c
  ggdkdrawlogger.c
  ggdkdrawP.h
  ggroupbox.c
  ghvbox.c
  gimageclut.c
  gimagecvt.c
  gkeysym.c
  glist.c
  gmatrixedit.c
  gmenu.c
  gprogress.c
  gradio.c
  gresedit.c
  gresource.c
  gresourceP.h
  gsavefiledlg.c
  gscrollbar.c
  gscroll1box.c
  gspacer.c
  gtabset.c
  gtextfield.c
  gtextinfo.c
  gwidgetP.h
  gwidgets.c
  hotkeys.c
)

target_link_libraries(gdraw PRIVATE fontforge)

if(APPLE)
  target_sources(gdraw PRIVATE ggdkcocoa.m)
endif()
target_link_libraries(gdraw PUBLIC
  GDK3::GDK3
  PkgConfig::GTK3
)
