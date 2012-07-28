# This file is included by automake for "normal" builds, and by
# GNUmakefile in GNUstep builds.

GUTL_VERSION = 1
GUTL_REVISION = 3
GUTL_AGE = 0

GFTP_VERSION = 1
GFTP_REVISION = 0
GFTP_AGE = 0

GUTILS_C_SRCFILES = divisors.c fsys.c gcol.c gimage.c gimagereadbmp.c	\
	gimageread.c gimagereadgif.c gimagereadjpeg.c gimagereadpng.c		\
	gimagereadras.c gimagereadrgb.c gimagereadtiff.c gimagereadxbm.c	\
	gimagereadxpm.c gimagewritebmp.c gimagewritegimage.c				\
	gimagewritejpeg.c gimagewritepng.c gimagewritexbm.c					\
	gimagewritexpm.c gwwintl.c dynamic.c gio.c giofile.c giohosts.c		\
	giomime.c giothread.c giotrans.c

GIOFTP_C_SRCFILES = gioftp.c

GUTILS_H_SRCFILES = gimagebmpP.h gioftpP.h giofuncP.h gioP.h
GIOFTP_H_SRCFILES = $(GUTILS_H_SRCFILES)
