# Makefile for OpenVMS
# Date : 11 November 2008

all : config.h
	@ write sys$output "configuration completed"

config.h : config.h_vms
	copy config.h_vms config.h
