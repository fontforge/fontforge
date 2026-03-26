@ECHO OFF
set "FF=%~dp0"
set FF_XPORT=11
set DISPLAY=127.0.0.1:%FF_XPORT%.0
set "XLOCALEDIR=%FF%bin\VcXsrv\locale"
set "XLOCALELIBDIR=%XLOCALEDIR%"
set AUTOTRACE=potrace
set "HOME=%FF%"
set FF_PORTABLE=TRUE

::Set this item to enable experimental Unicode filename support
::set LC_ALL=C.UTF-8

::Set this to your language code to change the FontForge UI language
::See share/locale/ for a list of supported language codes
::set LANGUAGE=en

::Only add to path once
if not defined FF_PATH_ADDED (
set "PATH=%FF%;%FF%\bin;%PATH:"=%"
set FF_PATH_ADDED=TRUE
)

echo Running FontForge in debug mode, expect it to be slower!

"%FF%\bin\VcXsrv_util.exe" -exists || (
start /B "" "%FF%\bin\VcXsrv\vcxsrv.exe" :%FF_XPORT% -multiwindow -clipboard -silent-dup-error
)

"%FF%\bin\VcXsrv_util.exe" -wait

"%FF%\bin\gdb.exe" --batch --command="%FF%\ffdebugscript.txt" --args "%FF%\bin\fontforge.exe" -nosplash %* 2>&1 | wtee "%TEMP%\FontForge-Debug-Output.txt"
explorer "%TEMP%\FontForge-Debug-Output.txt"

"%FF%\bin\VcXsrv_util.exe" -close
:: bye

