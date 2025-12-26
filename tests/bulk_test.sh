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

# FF_PY_SCRIPT_LINE="f=fontforge.open(argv[1]); f.generate(argv[2], flags='no-FFTM-table')"
# FF_OUTPUT_FILE="Output.ttf"

# FF_PY_SCRIPT_LINE="\
# f=fontforge.open(argv[1]);\
# f.generateFeatureFile('Out.fea')"$'\n'"\
# for l in f.gsub_lookups: f.removeLookup(l)"$'\n'"\
# for l in f.gpos_lookups: f.removeLookup(l)"$'\n'"\
# f.mergeFeature('Out.fea'); f.save(argv[2])"
# FF_OUTPUT_FILE="Output.sfd"

# # NOTE: Zeroize "CreationTime" and "ModificationTime" in SFD_DumpSplineFontMetadata()
# FF_PY_SCRIPT_LINE="\
# f=fontforge.open(argv[1]);\
# f.cidConvertByCmap('/home/iorsh/devel/cmap-resources/Adobe-Identity-0/CMap/Identity-H')"$'\n'"\
# f.generateFeatureFile('Out.fea')"$'\n'"\
# for l in f.gsub_lookups: f.removeLookup(l)"$'\n'"\
# for l in f.gpos_lookups: f.removeLookup(l)"$'\n'"\
# f.mergeFeature('Out.fea'); f.save(argv[2])"
# FF_OUTPUT_FILE="Output.sfd"

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
FF_BIN_PROJECT=$SCRIPT_DIR/$FF_BIN_PROJECT

cat $FONT_LIST |
while read font_url; do
    failure=0
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
            echo "\e[0;33mNO OUTPUT\e[0m: $font_url"
            failure=1
        elif (cmp --quiet $TMP_DIR_SYS/$FF_OUTPUT_FILE $TMP_DIR_PROJ/$FF_OUTPUT_FILE); then
            echo "PASS: $font_url"
        else
            echo -e "\e[0;31mFAIL\e[0m: $font_url"
            failure=1
        fi

        if [ $failure -eq 0 ]; then
            rm -rf $TMP_DIR_PROJ
        else
            echo "    Project output: $TMP_DIR_PROJ"
        fi
    else
        echo "SKIP: $font_url"
    fi
    popd > /dev/null
    if [ $failure -eq 0 ]; then
        rm -rf $TMP_DIR_SYS
    else
        echo "    System output: $TMP_DIR_SYS"
    fi
done
