# Makefile for OpenVMS
# Date : 4 January 2008

all :
	set def [.unicode]
	mms
	set def [-.gdraw]
	mms
	set def [-.gutils]
	mms
	set def [-.fontforge]
	mms
	set def [-.plugins]
	mms
