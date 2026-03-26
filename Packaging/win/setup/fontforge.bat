@ECHO OFF
set "FF=%~dp0"
set "HOME=%FF%"
set "PYTHONHOME=%FF%"
set AUTOTRACE=potrace

::Set this to your language code to change the FontForge UI language
::See share/locale/ for a list of supported language codes
::set LANGUAGE=en

::Only add to path once
if not defined FF_PATH_ADDED (
set "PATH=%FF%;%FF%\bin;%PATH:"=%"
set FF_PATH_ADDED=TRUE
)

for /F "tokens=* USEBACKQ" %%f IN (`dir /b "%FF%lib\python*"`) do (
set "PYTHONPATH=%FF%lib\%%f"
)

"%FF%\bin\fontforge.exe" -nosplash %*

:: bye

