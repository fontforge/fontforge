# Makefile for OpenVMS
# Date : 11 November 2008

all :
	set def [.inc]
	mms
	set def [-.unicode]
	mms
	set def [-.gdraw]
	mms
	set def [-.gutils]
	mms
	set def [-.fontforge]
	mms
	set def [-.plugins]
	mms
