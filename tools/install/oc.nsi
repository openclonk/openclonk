; TODO: PRODUCT_VERSION
!define PRODUCT_NAME "OpenClonk"
!define PRODUCT_PUBLISHER "OpenClonk Development Team"
!define PRODUCT_WEB_SITE "http://www.openclonk.org"
!define PRODUCT_WEB_SITE_NAME "OpenClonk Website"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_USER_KEY "Software\OpenClonk\OpenClonk"
!define PRODUCT_COMPANY_KEY "Software\OpenClonk"
!define PRODUCT_USER_ROOT_KEY "HKCU"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; zlib compression creates a corrupted installer!?
SetCompressor lzma

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ICON "inst.ico"
!define MUI_UNICON "uninst.ico"

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
;!define MUI_HEADERIMAGE_BITMAP "header.bmp"
;!define MUI_WELCOMEFINISHPAGE_BITMAP "welcome.bmp"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
;!define MUI_FINISHPAGE_RUN "$INSTDIR\Clonk.exe" Do not launch application after completion...
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "English"

; Overwrite language strings
LangString MUI_TEXT_WELCOME_INFO_TITLE ${LANG_German} "Willkommen zu ${PRODUCT_NAME}!"
LangString MUI_TEXT_WELCOME_INFO_TITLE ${LANG_English} "Welcome to ${PRODUCT_NAME}!"
LangString MUI_TEXT_FINISH_INFO_TITLE ${LANG_German} "Die Installation wird abgeschlossen."

; Additional language strings
LangString MUI_TEXT_USERPATH ${LANG_German} "Benutzerpfad"
LangString MUI_TEXT_USERPATH ${LANG_English} "User Path"

; Reserve files
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

; MUI end ------

Name "${PRODUCT_NAME}"
OutFile "..\oc.exe"
InstallDir "@PROGRAMFILES@\OpenClonk"
ShowInstDetails show
ShowUnInstDetails show

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite on

; Main program files  
  File "Clonk.exe"
  File "c4group.exe"

  File "*.dll"

  File "*.c4?"
  
  File "AUTHORS"
  File "COPYING"
  File "LGPL.txt"
  File "OpenSSL.txt"
  
; Create user path (works for the installing user only... might also want to put an info.txt dummy in there...)
  CreateDirectory "$APPDATA\OpenClonk"

; Create desktop shortcut
  CreateShortcut "$DESKTOP\OpenClonk.lnk" "$INSTDIR\Clonk.exe"

; Create website url in program directory
  WriteIniStr "$INSTDIR\${PRODUCT_WEB_SITE_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"

; Create user path shortcut in program directory
  CreateShortCut "$INSTDIR\$(MUI_TEXT_USERPATH).lnk" "%APPDATA%\OpenClonk"

; Copy key file from installer position - for cd-on-demand use (might want to move it directly into user path...)
;  CopyFiles /silent /filesonly "$EXEDIR\*.c4k" "$INSTDIR"

  ; Start menu shortcuts (All Users)
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\OpenClonk"
  CreateShortCut "$SMPROGRAMS\OpenClonk\OpenClonk.lnk" "$INSTDIR\Clonk.exe"
  CreateShortCut "$SMPROGRAMS\OpenClonk\${PRODUCT_WEB_SITE_NAME}.lnk" "$INSTDIR\${PRODUCT_WEB_SITE_NAME}.url"
  CreateShortCut "$SMPROGRAMS\OpenClonk\$(MUI_TEXT_USERPATH).lnk" "%APPDATA%\OpenClonk"
  CreateShortCut "$SMPROGRAMS\OpenClonk\Uninstall.lnk" "$INSTDIR\uninst.exe"

SectionEnd

Section -Post
; Uninstaller info
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\Clonk.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
; Register file types  
  WriteRegStr HKCR ".c4s" "" "Clonk4.Scenario"
  WriteRegStr HKCR ".c4s\Content Type" "" "vnd.clonk.c4group"
  WriteRegStr HKCR ".c4g" "" "Clonk4.Group"
  WriteRegStr HKCR ".c4g\Content Type" "" "vnd.clonk.c4group"
  WriteRegStr HKCR ".c4f" "" "Clonk4.Folder"
  WriteRegStr HKCR ".c4f\Content Type" "" "vnd.clonk.c4group"
  WriteRegStr HKCR ".c4p" "" "Clonk4.Player"
  WriteRegStr HKCR ".c4p\Content Type" "" "vnd.clonk.c4group"
  WriteRegStr HKCR ".c4d" "" "Clonk4.Definition"
  WriteRegStr HKCR ".c4d\Content Type" "" "vnd.clonk.c4group"
  WriteRegStr HKCR ".c4i" "" "Clonk4.Object"
  WriteRegStr HKCR ".c4i\Content Type" "" "vnd.clonk.c4group"
  WriteRegStr HKCR ".c4m" "" "Clonk4.Material"
  WriteRegStr HKCR ".c4m\Content Type" "" "text/plain"
  WriteRegStr HKCR ".c4b" "" "Clonk4.Binary"
  WriteRegStr HKCR ".c4b\Content Type" "" "application/octet-stream"
  WriteRegStr HKCR ".c4v" "" "Clonk4.Video"
  WriteRegStr HKCR ".c4v\Content Type" "" "video/avi"
  WriteRegStr HKCR ".c4l" "" "Clonk4.Weblink"
  WriteRegStr HKCR ".c4l\Content Type" "" "vnd.clonk.c4group"
  WriteRegStr HKCR ".c4k" "" "Clonk4.Key"
  WriteRegStr HKCR ".c4k\Content Type" "" "application/octet-stream"
  WriteRegStr HKCR ".c4u" "" "Clonk4.Update"
  WriteRegStr HKCR ".c4u\Content Type" "" "vnd.clonk.c4group"
; Register file classes  
  WriteRegStr HKCR "Clonk4.Scenario" "" "Clonk 4 Scenario"
  WriteRegStr HKCR "Clonk4.Scenario\DefaultIcon" "" "$INSTDIR\Clonk.exe,1"
  WriteRegStr HKCR "Clonk4.Group" "" "Clonk 4 Group"
  WriteRegStr HKCR "Clonk4.Group\DefaultIcon" "" "$INSTDIR\Clonk.exe,2"
  WriteRegStr HKCR "Clonk4.Folder" "" "Clonk 4 Folder"
  WriteRegStr HKCR "Clonk4.Folder\DefaultIcon" "" "$INSTDIR\Clonk.exe,3"
  WriteRegStr HKCR "Clonk4.Player" "" "Clonk 4 Player"
  WriteRegStr HKCR "Clonk4.Player\DefaultIcon" "" "$INSTDIR\Clonk.exe,4"
  WriteRegStr HKCR "Clonk4.Executable" "" "Clonk 4 Executable"
  WriteRegStr HKCR "Clonk4.Executable\DefaultIcon" "" "$INSTDIR\Clonk.exe,5"
  WriteRegStr HKCR "Clonk4.Definition" "" "Clonk 4 Object Definition"
  WriteRegStr HKCR "Clonk4.Definition\DefaultIcon" "" "$INSTDIR\Clonk.exe,6"
  WriteRegStr HKCR "Clonk4.Object" "" "Clonk 4 Object Info"
  WriteRegStr HKCR "Clonk4.Object\DefaultIcon" "" "$INSTDIR\Clonk.exe,7"
  WriteRegStr HKCR "Clonk4.Material" "" "Clonk 4 Material"
  WriteRegStr HKCR "Clonk4.Material\DefaultIcon" "" "$INSTDIR\Clonk.exe,8"
  WriteRegStr HKCR "Clonk4.Binary" "" "Clonk 4 Binary"
  WriteRegStr HKCR "Clonk4.Binary\DefaultIcon" "" "$INSTDIR\Clonk.exe,9"
  WriteRegStr HKCR "Clonk4.Video" "" "Clonk 4 Video"
  WriteRegStr HKCR "Clonk4.Video\DefaultIcon" "" "$INSTDIR\Clonk.exe,10"
  WriteRegStr HKCR "Clonk4.Weblink" "" "Clonk 4 Weblink"
  WriteRegStr HKCR "Clonk4.Weblink\DefaultIcon" "" "$INSTDIR\Clonk.exe,11"
  WriteRegStr HKCR "Clonk4.Key" "" "Clonk 4 Key"
  WriteRegStr HKCR "Clonk4.Key\DefaultIcon" "" "$INSTDIR\Clonk.exe,12"
  WriteRegStr HKCR "Clonk4.Update" "" "Clonk 4 Update"
  WriteRegStr HKCR "Clonk4.Update\DefaultIcon" "" "$INSTDIR\Clonk.exe,13"
; Register additional file handling
  WriteRegStr HKCR "Clonk4.Key\Shell\Register" "" "Register"
  WriteRegStr HKCR "Clonk4.Key\Shell\Register\Command" "" "$\"$INSTDIR\Clonk.exe$\" $\"%1$\""
  WriteRegStr HKCR "Clonk4.Update\Shell\Update" "" "Update"
  WriteRegStr HKCR "Clonk4.Update\Shell\Update\Command" "" "$\"$INSTDIR\Clonk.exe$\" $\"%1$\""
; Remove old use of App Paths   
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\App Paths\Clonk.exe"    
SectionEnd

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) wurde erfolgreich deinstalliert."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Sind Sie sicher, dass Sie $(^Name) deinstallieren wollen?" IDYES +2
  Abort
FunctionEnd

Section Uninstall

  ; Installation directory
  Delete "$INSTDIR\Clonk.exe"
  Delete "$INSTDIR\c4group.exe"

  Delete "$INSTDIR\*.dll"

  Delete "$INSTDIR\*.c4?"

  Delete "$INSTDIR\AUTHORS"
  Delete "$INSTDIR\COPYING"
  Delete "$INSTDIR\LGPL.txt"
  Delete "$INSTDIR\OpenSSL.txt"
  
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
  DeleteRegKey HKCR ".c4s"
  DeleteRegKey HKCR "Clonk4.Scenario"
  DeleteRegKey HKCR ".c4g"
  DeleteRegKey HKCR "Clonk4.Group"
  DeleteRegKey HKCR ".c4f"
  DeleteRegKey HKCR "Clonk4.Folder"
  DeleteRegKey HKCR ".c4p"
  DeleteRegKey HKCR "Clonk4.Player"
  DeleteRegKey HKCR ".c4x"
  DeleteRegKey HKCR "Clonk4.Executable"
  DeleteRegKey HKCR ".c4d"
  DeleteRegKey HKCR "Clonk4.Definition"
  DeleteRegKey HKCR ".c4i"
  DeleteRegKey HKCR "Clonk4.Object"
  DeleteRegKey HKCR ".c4m"
  DeleteRegKey HKCR "Clonk4.Material"
  DeleteRegKey HKCR ".c4b"
  DeleteRegKey HKCR "Clonk4.Binary"
  DeleteRegKey HKCR ".c4v"
  DeleteRegKey HKCR "Clonk4.Video"
  DeleteRegKey HKCR ".c4l"
  DeleteRegKey HKCR "Clonk4.Weblink"
  DeleteRegKey HKCR ".c4k"
  DeleteRegKey HKCR "Clonk4.Key"
  DeleteRegKey HKCR ".c4u"
  DeleteRegKey HKCR "Clonk4.Update"
  
  ; Start menu shortcuts (All Users)
  SetShellVarContext all
  Delete "$SMPROGRAMS\OpenClonk\*.lnk"
  RMDir "$SMPROGRAMS\OpenClonk"
  
  SetAutoClose true
SectionEnd
