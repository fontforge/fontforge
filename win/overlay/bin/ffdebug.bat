cd /d %~dp0
set DISPLAY=:0

gdb --batch --command="ffdebugscript.txt" fontforge.exe >%TEMP%\FontForge-Debug-Output.txt 2>&1
explorer %TEMP%\FontForge-Debug-Output.txt

