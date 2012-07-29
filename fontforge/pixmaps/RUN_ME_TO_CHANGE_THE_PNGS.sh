#!/bin/sh
#
# This is not done in the Makefile because it is not normally run by
# people doing builds. It is to be run by developers who are changing
# the png collection.
#
# Having the files listed explicitly rather than looking for them in
# the Makefile lets automake keep track of the distribution.

echo "PNG_LIST = "`ls *.png` > png_list.mk
