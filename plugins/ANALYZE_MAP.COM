$! Analyze Map for OpenVMS AXP
$!
$! Originally found in the distribution of gv
$!       http://wwwthep.physik.uni-mainz.de/~plass/gv/
$!
$! 1-Jul-1999 : modified to be used with $BSS$ & $READONLY sections in the
$!              map-file by J. Jansen (joukj@hrem.stm.tudelft.nl)
$!
$ SET SYMBOL/GENERAL/SCOPE=(NOLOCAL,NOGLOBAL)
$ SAY := "WRITE_ SYS$OUTPUT"
$ 
$ IF F$SEARCH("''P1'") .EQS. ""
$ THEN
$    SAY "  ANALYZE_MAP.COM:  Error, no mapfile provided"
$    EXIT_
$ ENDIF
$ IF "''P2'" .EQS. ""
$ THEN
$    SAY "  ANALYZE_MAP.COM:  Error, no output file provided"
$    EXIT_
$ ENDIF
$
$ LINK_TMP  = F$PARSE(P2,,,"DEVICE")+F$PARSE(P2,,,"DIRECTORY")+F$PARSE(P2,,,"NAME")+".TMP"
$
$ SAY "  checking PSECT list in ''P2'"
$ OPEN_/READ IN 'P1'
$ LOOP_PSECT_SEARCH:
$    READ_/END=EOF_PSECT IN REC
$ LOOP_PSECT_SEARCH0:
$    if F$EXTRACT(0,5,REC) .eqs. "$DATA" .or. F$EXTRACT(0,5,REC) .eqs. -
       "$BSS$" .or. f$extract(0,10,rec) .eqs. "$READONLY$" .or. -
       f$extract(0,9,rec) .eqs. "$LITERAL$" then goto do_data
$    if F$EXTRACT(0,14,REC) .eqs. "$READONLY_ADDR" then goto do_readonly
$    goto LOOP_PSECT_SEARCH
$ do_data:
$ LAST = ""
$ LOOP_PSECT:
$    READ_/END=EOF_PSECT IN REC
$    if F$EXTRACT(0,1,REC) .eqs. "$" .and. F$EXTRACT(0,5,REC) .nes. "$DATA" -
       .and. F$EXTRACT(0,5,REC) .nes. "$BSS$" .and.  f$extract(0,10,rec) -
       .nes. "$READONLY$" .and.  f$extract(0,9,rec) .nes. "$LITERAL$" then -
       goto LOOP_PSECT_SEARCH0
$    if REC - "NOPIC,OVR,REL,GBL,NOSHR,NOEXE,  WRT,NOVEC" .nes. REC
$    then 
$       J = F$LOCATE(" ",REC)
$       S = F$EXTRACT(0,J,REC)
$       IF S .EQS. LAST THEN GOTO LOOP_PSECT
$       P$_'S= 1
$       LAST = S
$    endif
$    if REC - "NOPIC,OVR,REL,GBL,NOSHR,NOEXE,NOWRT,NOVEC" .nes. REC
$    then 
$       J = F$LOCATE(" ",REC)
$       S = F$EXTRACT(0,J,REC)
$       IF S .EQS. LAST THEN GOTO LOOP_PSECT
$       P$_'S= 1
$       LAST = S
$    endif
$    GOTO LOOP_PSECT
$ 
$ do_readonly:
$ LAST = ""
$ LOOP_PSECT3:
$    READ_/END=EOF_PSECT IN REC
$    if F$EXTRACT(0,1,REC) .eqs. "-" .or. F$EXTRACT(0,3,REC) .eqs. "NL:" then -
       goto loop_psect3
$    if F$EXTRACT(0,1,REC) .eqs. "$" .and. F$EXTRACT(0,14,REC) .nes. -
       "$READONLY_ADDR" then goto LOOP_PSECT_SEARCH0
$    if REC - "OCTA" .nes. REC
$    then 
$       J = F$LOCATE(" ",REC)
$       S = F$EXTRACT(0,J,REC)
$       IF S .EQS. LAST THEN GOTO LOOP_PSECT3
$       P$_'S= 1
$       LAST = S
$    endif
$    GOTO LOOP_PSECT3
$
$ EOF_PSECT:
$    CLOSE_ IN
$
$ SAY "  appending list of UNIVERSAL procedures to ''P2'"
$ SEARCH_/NOHIGH/WINDOW=(0,0) 'P1' " R-"/OUT='LINK_TMP
$ OPEN_/READ IN 'LINK_TMP
$ OPEN_/write OUT 'P2'
$ WRITE_ OUT "!"
$ WRITE_ OUT "! ### UNIVERSAL procedures and global definitions extracted from ''P1'"
$ WRITE_ OUT "!"
$ write_ OUT "case_sensitive=YES"
$ LOOP_UNIVERSAL:
$    READ_/END=EOF_UNIVERSAL IN REC
$    J = F$LOCATE(" R-",REC)
$    S = F$EXTRACT(J+3,F$length(rec),REC)
$    J = F$LOCATE(" ",S)
$    S = F$EXTRACT(0,J,S)
$    PP$_'S= 1
$    IF F$TYPE(P$_'S').EQS.""
$    THEN
$       WRITE_ OUT "symbol_vector = ("+S+"	= PROCEDURE)"
$    ELSE
$       WRITE_ OUT "symbol_vector = ("+S+"	= DATA)"
$    ENDIF
$    GOTO LOOP_UNIVERSAL
$ EOF_UNIVERSAL:
$    CLOSE_ IN
$    CLOSE_ OUT
$!
$ SAY "  creating PSECT list in ''P2'"
$ OPEN_/READ IN 'P1'
$ OPEN_/append OUT 'P2'
$ WRITE_ OUT "!"
$ WRITE_ OUT "! ### PSECT list extracted from ''P1'"
$ WRITE_ OUT "!" 
$ LOOP_PSECT_SEARCH1:
$    READ_/END=EOF_PSECT1 IN REC
$    if F$EXTRACT(0,5,REC) .nes. "$DATA" .and. F$EXTRACT(0,5,REC) .nes. -
       "$BSS$" .and. f$extract(0,10,rec) .nes. "$READONLY$" .and. -
       f$extract(0,9,rec) .nes. "$LITERAL$" then goto LOOP_PSECT_SEARCH1
$ LAST = ""
$ LOOP_PSECT1:
$    READ_/END=EOF_PSECT1 IN REC
$    if F$EXTRACT(0,1,REC) .eqs. "$" .and. F$EXTRACT(0,5,REC) .nes. "$DATA" -
       .and. F$EXTRACT(0,5,REC) .nes. "$BSS$" .and.  f$extract(0,10,rec) -
       .nes. "$READONLY$"  .and. f$extract(0,9,rec) .nes. "$LITERAL$" then -
       goto LOOP_PSECT_SEARCH1
$    if REC - "NOPIC,OVR,REL,GBL,NOSHR,NOEXE,  WRT,NOVEC" .nes. REC
$    then 
$       J = F$LOCATE(" ",REC)
$       S = F$EXTRACT(0,J,REC)
$       IF S .EQS. LAST THEN GOTO LOOP_PSECT1
$       IF F$TYPE(PP$_'S').nes."" then WRITE_ OUT "symbol_vector = (" +  S + " = PSECT)"
$       P$_'S= 1
$       LAST = S
$    endif
$    if REC - "NOPIC,OVR,REL,GBL,NOSHR,NOEXE,NOWRT,NOVEC" .nes. REC
$    then 
$       J = F$LOCATE(" ",REC)
$       S = F$EXTRACT(0,J,REC)
$       IF S .EQS. LAST THEN GOTO LOOP_PSECT1
$       IF F$TYPE(PP$_'S').nes."" then WRITE_ OUT "symbol_vector = (" +  S + " = PSECT)"
$       P$_'S= 1
$       LAST = S
$    endif
$    GOTO LOOP_PSECT1
$
$ EOF_PSECT1:
$    CLOSE_ IN
$    CLOSE_ OUT 
$    if f$search("''LINK_TMP'") .nes. "" then DELETE_/NOLOG/NOCONFIRM 'LINK_TMP';*
$
$ EXIT_ 
