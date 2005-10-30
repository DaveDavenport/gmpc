; gmpc.nsi
;
; Gmpc installation script for NSIS installer compiler
; By: Daniel Lindenaar <daniel-gmpc@lindenaar.org>

;--------------------------------

Function .onInit
  Push $R0
  ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Gmpc" "UninstallString"
  StrCmp $R0 "" Gmpc_NOT_PRESENT
  MessageBox MB_OK "Gmpc is already installed. Installation will be aborted."
  Pop $R0
  Abort
  Gmpc_NOT_PRESENT:
  Pop $R0
FunctionEnd

; The name of the installer
Name "Gmpc"

; The file to write
OutFile "gmpc_win32_installer.exe"

; The default installation directory
InstallDir $PROGRAMFILES\gmpc
InstallDirRegKey HKLM "Software\gmpc" "InstallationDirectory"
;--------------------------------

; Pages

Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles
;--------------------------------

; The stuff to install
Section "Install" 
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Gmpc" "DisplayName" "Gmpc simulation tool"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Gmpc" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Gmpc" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Gmpc" "NoRepair" 1

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File src\gmpc.exe
  File libmpd-0.dll
  File libregex.dll
  SetOutPath $INSTDIR\data\glade
  File glade\*.glade
  SetOutPath $INSTDIR\data\images
  File pixmaps\*.png

  WriteRegStr HKLM "Software\gmpc" "InstallationDirectory" $INSTDIR
  WriteUninstaller "$INSTDIR\Uninstall.exe" 

SectionEnd ; end the section

Section "Uninstall"
  RMDir /r $INSTDIR
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Gmpc"
  DeleteRegKey HKLM "Software\gmpc" 
SectionEnd
