#!/bin/bash
#
# Run same operation using system FontForge version and built FontForge version
# and verify that the outputs are identical. Useful to perform regression testing
# on many fonts at once.

FONT_LIST=fonts/PublicFontList.txt
FF_BIN_SYSTEM=/usr/local/bin/fontforge
FF_BIN_PROJECT=../build/bin/fontforge

# Python commands to be executed for each tested SFD file.
# Arguments:
#       argv[1] - input SFD file
#       argv[2] - output file to be compared bytewise
FF_PY_SCRIPT_LINE="f=fontforge.open(argv[1]); f.save(argv[2])"
FF_OUTPUT_FILE="Output.sfd"

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
FF_BIN_PROJECT=$SCRIPT_DIR/$FF_BIN_PROJECT

cat $FONT_LIST |
while read font_url; do
    TMP_DIR_SYS=`mktemp -d`
    pushd $TMP_DIR_SYS > /dev/null
    if (wget -q --timeout=60 $font_url); then
        SFD_FILE=`ls`
        TMP_DIR_PROJ=`mktemp -d`
        cp $SFD_FILE $TMP_DIR_PROJ

        # Execute system FontForge binary
        $FF_BIN_SYSTEM -c "$FF_PY_SCRIPT_LINE" $SFD_FILE $FF_OUTPUT_FILE >/dev/null 2>&1

        # Execute compiled project FontForge binary
        pushd $TMP_DIR_PROJ > /dev/null
        $FF_BIN_PROJECT -c "$FF_PY_SCRIPT_LINE" $SFD_FILE $FF_OUTPUT_FILE >/dev/null 2>&1
        popd > /dev/null

        if [ ! -f $TMP_DIR_SYS/$FF_OUTPUT_FILE ] && [ ! -f $TMP_DIR_PROJ/$FF_OUTPUT_FILE ]; then
            echo "NO OUTPUT: $font_url"
        elif (cmp --quiet $TMP_DIR_SYS/$FF_OUTPUT_FILE $TMP_DIR_PROJ/$FF_OUTPUT_FILE); then
            echo "PASS: $font_url"
        else
            echo -e "\e[0;31mFAIL\e[0m: $font_url"
        fi

        rm -rf $TMP_DIR_PROJ
    else
        echo "SKIP: $font_url"
    fi
    popd > /dev/null
    rm -rf $TMP_DIR_SYS
done
