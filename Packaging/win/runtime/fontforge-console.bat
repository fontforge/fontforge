@echo off
echo Configuring the system path to add FontForge...
set FF=%~dp0
set "PYTHONHOME=%FF%"

if not defined FF_PATH_ADDED (
set "PATH=%FF%;%FF%\bin;%PATH:"=%"
set FF_PATH_ADDED=TRUE
)

for /F "tokens=* USEBACKQ" %%f IN (`dir /b "%FF%lib\python*"`) do (
set "PYTHONPATH=%FF%lib\%%f"
)

echo Configuration complete. You can now call 'fontforge' from the console.
echo You may also use the bundled Python distribution by calling `ffpython`.
echo Extra Python modules that you require may be installed via `ffpython`.
echo.
cmd /k