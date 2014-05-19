; fontforge.nsi
;
;
;--------------------------------

; The name of the installer
Name "FontForge"

; The file to write
OutFile "FontForge-setup.exe"

; The default installation directory
InstallDir $PROGRAMFILES\FontForge

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\NSIS_FontForge" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

Icon fontforge-installer-icon.ico

SetCompressor /SOLID lzma

;--------------------------------

; Pages

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "FontForge (required)"

  SectionIn RO
  SetShellVarContext all
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; What files to install
  File /r ../expanded/*.*
 
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\NSIS_FontForge "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FontForge" "DisplayName" "NSIS FontForge"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FontForge" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FontForge" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FontForge" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\FontForge"
  CreateShortCut "$SMPROGRAMS\FontForge\FontForge.lnk" "$INSTDIR\usr\i686-w64-mingw32\sys-root\mingw\bin\ff.bat" "" "$INSTDIR\usr\i686-w64-mingw32\sys-root\mingw\bin\ff.ico" 0 SW_SHOWMINIMIZED
  CreateShortCut "$SMPROGRAMS\FontForge\FontForge Debug.lnk" "$INSTDIR\usr\i686-w64-mingw32\sys-root\mingw\bin\ffdebug.bat" "" "$INSTDIR\usr\i686-w64-mingw32\sys-root\mingw\bin\ffdebug.ico" 0 SW_SHOWMINIMIZED
  CreateShortCut "$SMPROGRAMS\FontForge\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  SetShellVarContext all

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FontForge"
  DeleteRegKey HKLM SOFTWARE\NSIS_FontForge

  ; Remove files and uninstaller
  Delete $INSTDIR\uninstall.exe
  RMDir /r /REBOOTOK $INSTDIR
  RMDir /r /REBOOTOK "$SMPROGRAMS\FontForge"

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\FontForge\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\FontForge"
  RMDir "$INSTDIR"

SectionEnd
