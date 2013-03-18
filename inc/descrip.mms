# Makefile for OpenVMS
# Date : 11 November 2008

all : fontforge-config.h
	@ write sys$output "configuration completed"

fontforge-config.h : fontforge-config.h_vms
	copy fontforge-config.h_vms fontforge-config.h
