#!/bin/sh
#
# Having the files listed explicitly rather than looking for them in
# the Makefile lets automake keep track of the distribution.
#
# Maintainers with working Makefiles can simply delete png_list.mk to
# have it rebuilt by this script.

echo "PNG_LIST = "`ls *.png` > png_list.mk
