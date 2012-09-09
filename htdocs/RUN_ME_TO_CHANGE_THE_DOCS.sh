#!/bin/sh
#
# Having the files listed explicitly rather than looking for them in
# the Makefile lets automake keep track of the distribution.

doc_list="`find . -type f -print | grep -E -v '^.*(~|\.orig)$'`"
root_doc_list="`ls *.html *.png *.gif *.pdf`"
nonbmp_doc_list="`find nonBMP -type f -print`"
flags_doc_list="`find flags -type f -print`"
ja_doc_list="`find ja -type f -print`"
installed_doc_list="${root_doc_list} ${nonbmp_doc_list} ${flags_doc_list} ${ja_doc_list}"

echo "DOC_LIST = "${doc_list} > doc_list.mk
echo >> doc_list.mk
echo "ROOT_DOC_LIST = "${root_doc_list} >> doc_list.mk
echo >> doc_list.mk
echo "NONBMP_DOC_LIST = "${nonbmp_doc_list} >> doc_list.mk
echo >> doc_list.mk
echo "FLAGS_DOC_LIST = "${flags_doc_list} >> doc_list.mk
echo >> doc_list.mk
echo "JA_DOC_LIST = "${ja_doc_list} >> doc_list.mk
echo >> doc_list.mk
echo "INSTALLED_DOC_LIST = "${installed_doc_list} >> doc_list.mk
