!ifndef PRODUCT_NAME
!define PRODUCT_NAME "OpenClonk"
!endif
!ifndef PRODUCT_COMPANY
!define PRODUCT_COMPANY "OpenClonk"
!endif
!ifndef CLONK
!define CLONK "openclonk.exe"
!endif
!ifndef C4GROUP
!define C4GROUP "c4group.exe"
!endif
!define PRODUCT_FILENAME "openclonk.exe"
!define PRODUCT_PUBLISHER "OpenClonk Development Team"
!define PRODUCT_WEB_SITE "http://www.openclonk.org"
!define PRODUCT_WEB_SITE_NAME "OpenClonk Website"
!define PRODUCT_INSTDIR "OpenClonk"
!define UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\"
!define PRODUCT_USER_KEY "Software\${PRODUCT_COMPANY}\OpenClonk"
!define PRODUCT_COMPANY_KEY "Software\${PRODUCT_COMPANY}"

Name "${PRODUCT_NAME}"
SetCompressor lzma

; search paths
!addplugindir "${SRCDIR}/tools/install"
!addplugindir "tools/install"
!addincludedir "${SRCDIR}/tools/install"

; MultiUser Settings
!define MULTIUSER_EXECUTIONLEVEL Highest
;!define MULTIUSER_MUI
!define MULTIUSER_INSTALLMODE_COMMANDLINE
!define MULTIUSER_INSTALLMODE_INSTDIR "${PRODUCT_INSTDIR}"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_KEY "${PRODUCT_USER_KEY}"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME "InstallLocation"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_KEY "${PRODUCT_USER_KEY}"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_VALUENAME "InstallLocation"
!include MultiUser_x64.nsh

Function .onInit
  !insertmacro MULTIUSER_INIT
FunctionEnd

Function un.onInit
  !insertmacro MULTIUSER_UNINIT
FunctionEnd

; MUI Settings
!include MUI2.nsh

!define MUI_ICON "${SRCDIR}/tools/install/inst.ico"
!define MUI_UNICON "${SRCDIR}/tools/install/uninst.ico"

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP "${SRCDIR}/tools/install\header.bmp"

; Music pack installation routines
!include musicpack.nsh

; Installer pages
;!insertmacro MULTIUSER_PAGE_INSTALLMODE
!insertmacro MUI_PAGE_DIRECTORY

; Ask user to download music pack.
!insertmacro MusicPackChoice

!insertmacro MUI_PAGE_INSTFILES

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "German"

; Additional language strings
LangString OC_TEXT_USERPATH ${LANG_German} "Benutzerpfad"
LangString OC_TEXT_USERPATH ${LANG_English} "User Path"

;!insertmacro MUI_RESERVEFILE_LANGDLL
;ReserveFile "${NSISDIR}\Plugins\*.dll"
; MUI end ------

; Game Explorer
!include GameExplorer.nsh

ShowInstDetails show
ShowUnInstDetails show

Section
  SetOutPath "$INSTDIR"
  SetOverwrite on

  ; Main program files
  File /oname=${PRODUCT_FILENAME} "${CLONK}"
  File /oname=c4group.exe "${C4GROUP}"

  File "*.dll"

  SetOutPath "$INSTDIR\platforms"
  File "platforms\"

  SetOutPath "$INSTDIR"
  
  ; Delete any previously unpacked Music.ocg, which would block creation of the file.
  RMDir /r "$INSTDIR\Music.ocg"

  File "*.oc?"
  
  ; delete obsolete folders
  ; from 1.0
  Delete "BackToTheRocks.ocf"
  ; from 2.0
  Delete "BeyondTheRocks.ocf"
  ; from 3.0
  Delete "Settlement.ocf"
  ; from 4.0 (these accidentally got in in release, bug #1029)
  Delete "Issues.ocf"
  Delete "Experimental.ocd"
  Delete "Experimental.ocf"
  ; from 6.0 (got renamed to Tutorials.ocf)
  Delete "Tutorial.ocf"

  File "${SRCDIR}\planet\AUTHORS"
  File "${SRCDIR}\planet\COPYING"
  File "${SRCDIR}\licenses\LGPL.txt"
  File "${SRCDIR}\Credits.txt"

  ; Create user path (works for the installing user only... might also want to put an info.txt dummy in there...)
  CreateDirectory "$APPDATA\OpenClonk"

  ; Create website url in program directory
  WriteIniStr "$INSTDIR\${PRODUCT_WEB_SITE_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"

  ; Game Explorer integration
  ReadINIStr $0 $INSTDIR\GameExplorer.txt GameExplorer InstanceID
  IfErrors 0 UpdateInstallation
    ${GameExplorer_GenerateGUID}
    Pop $0
    WriteIniStr $INSTDIR\GameExplorer.txt GameExplorer InstanceID $0
    ${GameExplorer_AddGame} $MultiUser.InstallMode $INSTDIR\${PRODUCT_FILENAME} $INSTDIR $INSTDIR\${PRODUCT_FILENAME} $0
    IfErrors EndGameExplorer 0
    ; Create tasks.
    CreateDirectory $APPDATA\Microsoft\Windows\GameExplorer\$0\PlayTasks\0
    CreateShortcut $APPDATA\Microsoft\Windows\GameExplorer\$0\PlayTasks\0\Play.lnk "$INSTDIR\${PRODUCT_FILENAME}"
    CreateDirectory $APPDATA\Microsoft\Windows\GameExplorer\$0\PlayTasks\1
    CreateShortcut $APPDATA\Microsoft\Windows\GameExplorer\$0\PlayTasks\1\Editor.lnk "$INSTDIR\${PRODUCT_FILENAME}" --editor
    CreateDirectory $APPDATA\Microsoft\Windows\GameExplorer\$0\SupportTasks\0
    CreateShortcut "$APPDATA\Microsoft\Windows\GameExplorer\$0\SupportTasks\0\${PRODUCT_WEB_SITE_NAME}.lnk" "$INSTDIR\${PRODUCT_WEB_SITE_NAME}.url"
    goto EndGameExplorer
  UpdateInstallation:
    ${GameExplorer_UpdateGame} $0
  EndGameExplorer:

  ; Create desktop shortcut
  CreateShortcut "$DESKTOP\OpenClonk.lnk" "$INSTDIR\${PRODUCT_FILENAME}"

  ; Create user path shortcut in program directory
  CreateShortCut "$INSTDIR\$(OC_TEXT_USERPATH).lnk" "%APPDATA%\OpenClonk"

  ; Start menu shortcuts
  CreateDirectory "$SMPROGRAMS\OpenClonk"
  CreateShortCut "$SMPROGRAMS\OpenClonk\OpenClonk.lnk" "$INSTDIR\${PRODUCT_FILENAME}"
  CreateShortCut "$SMPROGRAMS\OpenClonk\OpenClonk Editor.lnk" "$INSTDIR\${PRODUCT_FILENAME}" "--editor"
  CreateShortCut "$SMPROGRAMS\OpenClonk\${PRODUCT_WEB_SITE_NAME}.lnk" "$INSTDIR\${PRODUCT_WEB_SITE_NAME}.url"
  CreateShortCut "$SMPROGRAMS\OpenClonk\$(OC_TEXT_USERPATH).lnk" "%APPDATA%\OpenClonk"

  ; Uninstaller info
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr SHELL_CONTEXT "${UNINST_KEY}$0" "DisplayName" "$(^Name)"
  WriteRegStr SHELL_CONTEXT "${UNINST_KEY}$0" "UninstallString" \
    "$\"$INSTDIR\uninst.exe$\" /$MultiUser.InstallMode"
  WriteRegStr SHELL_CONTEXT "${UNINST_KEY}$0" "QuietUninstallString" \
    "$\"$INSTDIR\uninst.exe$\" /$MultiUser.InstallMode /S"
  WriteRegStr SHELL_CONTEXT "${UNINST_KEY}$0" "DisplayIcon" "$\"$INSTDIR\${PRODUCT_FILENAME}$\""
  WriteRegDWORD SHELL_CONTEXT "${UNINST_KEY}$0" "NoModify" 1
  WriteRegDWORD SHELL_CONTEXT "${UNINST_KEY}$0" "NoRepair" 1
  WriteRegStr SHELL_CONTEXT "${UNINST_KEY}$0" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr SHELL_CONTEXT "${UNINST_KEY}$0" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr SHELL_CONTEXT "${UNINST_KEY}$0" "InstallLocation" "$INSTDIR"
  WriteRegStr SHELL_CONTEXT "${PRODUCT_USER_KEY}" "InstallLocation" "$INSTDIR"
  ; Register file types
  WriteRegStr HKCR ".ocs" "" "OpenClonk.Scenario"
  WriteRegStr HKCR ".ocs\Content Type" "" "vnd.clonk.c4group"
  WriteRegStr HKCR ".ocg" "" "OpenClonk.Group"
  WriteRegStr HKCR ".ocg\Content Type" "" "vnd.clonk.c4group"
  WriteRegStr HKCR ".ocf" "" "OpenClonk.Folder"
  WriteRegStr HKCR ".ocf\Content Type" "" "vnd.clonk.c4group"
  WriteRegStr HKCR ".ocp" "" "OpenClonk.Player"
  WriteRegStr HKCR ".ocp\Content Type" "" "vnd.clonk.c4group"
  WriteRegStr HKCR ".ocd" "" "OpenClonk.Definition"
  WriteRegStr HKCR ".ocd\Content Type" "" "vnd.clonk.c4group"
  WriteRegStr HKCR ".oci" "" "OpenClonk.Object"
  WriteRegStr HKCR ".oci\Content Type" "" "vnd.clonk.c4group"
  WriteRegStr HKCR ".ocm" "" "OpenClonk.Material"
  WriteRegStr HKCR ".ocm\Content Type" "" "text/plain"
  WriteRegStr HKCR ".ocb" "" "OpenClonk.Binary"
  WriteRegStr HKCR ".ocb\Content Type" "" "application/octet-stream"
  WriteRegStr HKCR ".ocv" "" "OpenClonk.Video"
  WriteRegStr HKCR ".ocv\Content Type" "" "video/avi"
  WriteRegStr HKCR ".ocl" "" "OpenClonk.Weblink"
  WriteRegStr HKCR ".ocl\Content Type" "" "vnd.clonk.c4group"
  WriteRegStr HKCR ".ocu" "" "OpenClonk.Update"
  WriteRegStr HKCR ".ocu\Content Type" "" "vnd.clonk.c4group"

  ; Register file classes
  WriteRegStr HKCR "OpenClonk.Scenario" "" "OpenClonk Scenario"
  WriteRegStr HKCR "OpenClonk.Scenario\DefaultIcon" "" "$INSTDIR\${PRODUCT_FILENAME},1"
  WriteRegStr HKCR "OpenClonk.Group" "" "OpenClonk Group"
  WriteRegStr HKCR "OpenClonk.Group\DefaultIcon" "" "$INSTDIR\${PRODUCT_FILENAME},2"
  WriteRegStr HKCR "OpenClonk.Folder" "" "OpenClonk Folder"
  WriteRegStr HKCR "OpenClonk.Folder\DefaultIcon" "" "$INSTDIR\${PRODUCT_FILENAME},3"
  WriteRegStr HKCR "OpenClonk.Player" "" "OpenClonk Player"
  WriteRegStr HKCR "OpenClonk.Player\DefaultIcon" "" "$INSTDIR\${PRODUCT_FILENAME},4"
  WriteRegStr HKCR "OpenClonk.Definition" "" "OpenClonk Object Definition"
  WriteRegStr HKCR "OpenClonk.Definition\DefaultIcon" "" "$INSTDIR\${PRODUCT_FILENAME},5"
  WriteRegStr HKCR "OpenClonk.Object" "" "OpenClonk Object Info"
  WriteRegStr HKCR "OpenClonk.Object\DefaultIcon" "" "$INSTDIR\${PRODUCT_FILENAME},6"
  WriteRegStr HKCR "OpenClonk.Material" "" "OpenClonk Material"
  WriteRegStr HKCR "OpenClonk.Material\DefaultIcon" "" "$INSTDIR\${PRODUCT_FILENAME},7"
  WriteRegStr HKCR "OpenClonk.Binary" "" "OpenClonk Binary"
  WriteRegStr HKCR "OpenClonk.Binary\DefaultIcon" "" "$INSTDIR\${PRODUCT_FILENAME},8"
  WriteRegStr HKCR "OpenClonk.Video" "" "OpenClonk Video"
  WriteRegStr HKCR "OpenClonk.Video\DefaultIcon" "" "$INSTDIR\${PRODUCT_FILENAME},9"
  WriteRegStr HKCR "OpenClonk.Weblink" "" "OpenClonk Weblink"
  WriteRegStr HKCR "OpenClonk.Weblink\DefaultIcon" "" "$INSTDIR\${PRODUCT_FILENAME},10"
  WriteRegStr HKCR "OpenClonk.Update" "" "OpenClonk Update"
  WriteRegStr HKCR "OpenClonk.Update\DefaultIcon" "" "$INSTDIR\${PRODUCT_FILENAME},11"

  ; Register additional file handling
  WriteRegStr HKCR "OpenClonk.Update\Shell\Update" "" "Update"
  WriteRegStr HKCR "OpenClonk.Update\Shell\Update\Command" "" "$\"$INSTDIR\${PRODUCT_FILENAME}$\" $\"%1$\""

  ; Add a Firewall exception
  firewall::AddAuthorizedApplication "$INSTDIR\${PRODUCT_FILENAME}" "$(^Name)"
  
  ; Download and Install extra music pack
  ; Do this after the other installation tasks so everything is in order if the user cancels during downloading.
  Call MusicPackDownload
  Call MusicPackInstall
  
  ; Unpack Music.ocg for faster music switching (regardless if it's the regular or the extended pack)
  DetailPrint "Unpacking Music file..."
  ExecShell "open" "$INSTDIR\c4group.exe" "Music.ocg -u" SW_HIDE

SectionEnd


Section Uninstall
  ; Game Explorer
  ReadINIStr $0 $INSTDIR\GameExplorer.txt GameExplorer InstanceID
  IfErrors NoGameExplorer 0
    ${GameExplorer_RemoveGame} $0
  NoGameExplorer:

  ; Installation directory
  Delete "$INSTDIR\${PRODUCT_FILENAME}"
  Delete "$INSTDIR\c4group.exe"

  Delete "$INSTDIR\*.dll"
  RMDir /r "$INSTDIR\platforms"

  ; Music may or may not be unpacked.
  RMDir /r "$INSTDIR\Music.ocg"
  Delete "$INSTDIR\*.oc?"

  Delete "$INSTDIR\AUTHORS"
  Delete "$INSTDIR\COPYING"
  Delete "$INSTDIR\LGPL.txt"
  Delete "$INSTDIR\OpenSSL.txt"  ; For installations up to 5.2.x
  Delete "$INSTDIR\Credits.txt"

  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\GameExplorer.txt"
  Delete "$INSTDIR\${PRODUCT_WEB_SITE_NAME}.url"
  Delete "$INSTDIR\$(OC_TEXT_USERPATH).lnk"

  RMDir "$INSTDIR"

  ; Desktop shortcut
  Delete "$DESKTOP\OpenClonk.lnk"

  ; Start menu shortcuts
  Delete "$SMPROGRAMS\OpenClonk\*.lnk"
  RMDir "$SMPROGRAMS\OpenClonk"
	
  ; Registry: config
  DeleteRegKey HKCU "${PRODUCT_USER_KEY}"
  DeleteRegKey /ifempty HKCU "${PRODUCT_COMPANY_KEY}"
  ; Registry: Uninstaller info
  DeleteRegKey SHELL_CONTEXT "${UNINST_KEY}$0"
  DeleteRegKey SHELL_CONTEXT "${PRODUCT_USER_KEY}"
  DeleteRegKey /ifempty SHELL_CONTEXT "${PRODUCT_COMPANY_KEY}"

  ; Registry: classes
  DeleteRegKey HKCR ".ocs"
  DeleteRegKey HKCR "OpenClonk.Scenario"
  DeleteRegKey HKCR ".ocg"
  DeleteRegKey HKCR "OpenClonk.Group"
  DeleteRegKey HKCR ".ocf"
  DeleteRegKey HKCR "OpenClonk.Folder"
  DeleteRegKey HKCR ".ocp"
  DeleteRegKey HKCR "OpenClonk.Player"
  DeleteRegKey HKCR ".ocd"
  DeleteRegKey HKCR "OpenClonk.Definition"
  DeleteRegKey HKCR ".oci"
  DeleteRegKey HKCR "OpenClonk.Object"
  DeleteRegKey HKCR ".ocm"
  DeleteRegKey HKCR "OpenClonk.Material"
  DeleteRegKey HKCR ".ocb"
  DeleteRegKey HKCR "OpenClonk.Binary"
  DeleteRegKey HKCR ".ocv"
  DeleteRegKey HKCR "OpenClonk.Video"
  DeleteRegKey HKCR ".ocl"
  DeleteRegKey HKCR "OpenClonk.Weblink"
  DeleteRegKey HKCR ".ocu"
  DeleteRegKey HKCR "OpenClonk.Update"

  ; Remove the Firewall exception
  firewall::RemoveAuthorizedApplication "$INSTDIR\${PRODUCT_FILENAME}"

SectionEnd
