!ifndef PRODUCT_NAME
!define PRODUCT_NAME "OpenClonk"
!endif
!define PRODUCT_PUBLISHER "OpenClonk Development Team"
!define PRODUCT_WEB_SITE "http://www.openclonk.org"
!define PRODUCT_WEB_SITE_NAME "OpenClonk Website"
!define PRODUCT_INSTDIR "OpenClonk"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_INSTDIR}"
!define PRODUCT_USER_KEY "Software\OpenClonk\OpenClonk"
!define PRODUCT_COMPANY_KEY "Software\OpenClonk"
!define PRODUCT_USER_ROOT_KEY "HKCU"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

Name "${PRODUCT_NAME}"
SetCompressor lzma

; MUI Settings
!include MUI2.nsh

!define MUI_ICON "${SRCDIR}/tools/install/inst.ico"
!define MUI_UNICON "${SRCDIR}/tools/install/uninst.ico"

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP "${SRCDIR}/tools/install/header.bmp"

; Installer pages
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "English"

; Additional language strings
LangString MUI_TEXT_USERPATH ${LANG_German} "Benutzerpfad"
LangString MUI_TEXT_USERPATH ${LANG_English} "User Path"

!insertmacro MUI_RESERVEFILE_LANGDLL
ReserveFile "${NSISDIR}\Plugins\*.dll"
; MUI end ------

InstallDir "${PROGRAMFILES}\OpenClonk"
ShowInstDetails show
ShowUnInstDetails show

Section
  SetOutPath "$INSTDIR"
  SetOverwrite on

; Main program files  
  File "Clonk.exe"
  File "c4group.exe"

  File "*.dll"

  File "*.oc?"
  
  File "${SRCDIR}\planet\AUTHORS"
  File "${SRCDIR}\planet\COPYING"
  File "${SRCDIR}\licenses\LGPL.txt"
  File "${SRCDIR}\licenses\OpenSSL.txt"
  File "${SRCDIR}\Credits.txt"
  
; Create user path (works for the installing user only... might also want to put an info.txt dummy in there...)
  CreateDirectory "$APPDATA\OpenClonk"

; Create desktop shortcut
  CreateShortcut "$DESKTOP\OpenClonk.lnk" "$INSTDIR\Clonk.exe"

; Create website url in program directory
  WriteIniStr "$INSTDIR\${PRODUCT_WEB_SITE_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"

; Create user path shortcut in program directory
  CreateShortCut "$INSTDIR\$(MUI_TEXT_USERPATH).lnk" "%APPDATA%\OpenClonk"

  ; Start menu shortcuts (All Users)
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\OpenClonk"
  CreateShortCut "$SMPROGRAMS\OpenClonk\OpenClonk.lnk" "$INSTDIR\Clonk.exe"
  CreateShortCut "$SMPROGRAMS\OpenClonk\OpenClonk Editor.lnk" "$INSTDIR\Clonk.exe" "--editor"
  CreateShortCut "$SMPROGRAMS\OpenClonk\${PRODUCT_WEB_SITE_NAME}.lnk" "$INSTDIR\${PRODUCT_WEB_SITE_NAME}.url"
  CreateShortCut "$SMPROGRAMS\OpenClonk\$(MUI_TEXT_USERPATH).lnk" "%APPDATA%\OpenClonk"

; Uninstaller info
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\Clonk.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
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
  WriteRegStr HKCR "OpenClonk.Scenario\DefaultIcon" "" "$INSTDIR\Clonk.exe,1"
  WriteRegStr HKCR "OpenClonk.Group" "" "OpenClonk Group"
  WriteRegStr HKCR "OpenClonk.Group\DefaultIcon" "" "$INSTDIR\Clonk.exe,2"
  WriteRegStr HKCR "OpenClonk.Folder" "" "OpenClonk Folder"
  WriteRegStr HKCR "OpenClonk.Folder\DefaultIcon" "" "$INSTDIR\Clonk.exe,3"
  WriteRegStr HKCR "OpenClonk.Player" "" "OpenClonk Player"
  WriteRegStr HKCR "OpenClonk.Player\DefaultIcon" "" "$INSTDIR\Clonk.exe,4"
  WriteRegStr HKCR "OpenClonk.Definition" "" "OpenClonk Object Definition"
  WriteRegStr HKCR "OpenClonk.Definition\DefaultIcon" "" "$INSTDIR\Clonk.exe,5"
  WriteRegStr HKCR "OpenClonk.Object" "" "OpenClonk Object Info"
  WriteRegStr HKCR "OpenClonk.Object\DefaultIcon" "" "$INSTDIR\Clonk.exe,6"
  WriteRegStr HKCR "OpenClonk.Material" "" "OpenClonk Material"
  WriteRegStr HKCR "OpenClonk.Material\DefaultIcon" "" "$INSTDIR\Clonk.exe,7"
  WriteRegStr HKCR "OpenClonk.Binary" "" "OpenClonk Binary"
  WriteRegStr HKCR "OpenClonk.Binary\DefaultIcon" "" "$INSTDIR\Clonk.exe,8"
  WriteRegStr HKCR "OpenClonk.Video" "" "OpenClonk Video"
  WriteRegStr HKCR "OpenClonk.Video\DefaultIcon" "" "$INSTDIR\Clonk.exe,9"
  WriteRegStr HKCR "OpenClonk.Weblink" "" "OpenClonk Weblink"
  WriteRegStr HKCR "OpenClonk.Weblink\DefaultIcon" "" "$INSTDIR\Clonk.exe,10"
  WriteRegStr HKCR "OpenClonk.Update" "" "OpenClonk Update"
  WriteRegStr HKCR "OpenClonk.Update\DefaultIcon" "" "$INSTDIR\Clonk.exe,11"
; Register additional file handling
  WriteRegStr HKCR "OpenClonk.Update\Shell\Update" "" "Update"
  WriteRegStr HKCR "OpenClonk.Update\Shell\Update\Command" "" "$\"$INSTDIR\Clonk.exe$\" $\"%1$\""
; Remove old use of App Paths   
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\App Paths\Clonk.exe"    

SectionEnd


Section Uninstall

  ; Installation directory
  Delete "$INSTDIR\Clonk.exe"
  Delete "$INSTDIR\c4group.exe"

  Delete "$INSTDIR\*.dll"

  Delete "$INSTDIR\*.oc?"

  Delete "$INSTDIR\AUTHORS"
  Delete "$INSTDIR\COPYING"
  Delete "$INSTDIR\LGPL.txt"
  Delete "$INSTDIR\OpenSSL.txt"
  Delete "$INSTDIR\Credits.txt"
  
  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\${PRODUCT_WEB_SITE_NAME}.url"
  Delete "$INSTDIR\$(MUI_TEXT_USERPATH).lnk"

  RMDir "$INSTDIR"

  ; Desktop shortcut
  Delete "$DESKTOP\OpenClonk.lnk"
	
  ; Registry: config
  DeleteRegKey ${PRODUCT_USER_ROOT_KEY} "${PRODUCT_USER_KEY}"
  DeleteRegKey /ifempty ${PRODUCT_USER_ROOT_KEY} "${PRODUCT_COMPANY_KEY}"
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  
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
  
  ; Start menu shortcuts (All Users)
  SetShellVarContext all
  Delete "$SMPROGRAMS\OpenClonk\*.lnk"
  RMDir "$SMPROGRAMS\OpenClonk"
  
SectionEnd
