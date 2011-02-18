# Makefile for OpenVMS
# Date : 16 February 2011

all :
	if f$search("libs.dir") .eqs. "" then create/directory [.libs]
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
