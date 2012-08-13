#!/bin/sh
#
# Having the files listed explicitly rather than looking for them in
# the Makefile lets automake keep track of the distribution.

echo "PNG_LIST = "`ls *.png` > png_list.mk
