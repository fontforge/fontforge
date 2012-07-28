#!/bin/sh
#
# This is not done in the Makefile because it is not meant for people
# doing builds. It is to be run by developers who are changing the png
# collection.

echo "PNG_LIST = `ls *.png`" | tr '\n' ' ' > png_list.mk
