; Defines functions for extra music pack download:
; MusicPackChoice - shows page with checkbox to download music pack
; MusicPackDownload - downloads music pack
; MusicPackInstall - copies downloaded music pack into game folder

!include nsDialogs.nsh
!include LogicLib.nsh

!define MUSICPACK_URL "http://www.openclonk.org/download/Music.ocg"
; !define MUSICPACK_URL "http://www.cognium.de/misc/test.ocg"

Var Dialog
Var Label
Var MusicCheckbox
Var MusicCheckbox_State
Var TempMusicFn

!macro MusicPackChoice
	Page custom nsMusicPage nsMusicPageDone
!macroend

Function nsMusicPage

	nsDialogs::Create 1018
	Pop $Dialog

	${If} $Dialog == error
		Abort
	${EndIf}
	
	!insertmacro MUI_HEADER_TEXT "Additional music package" "Download additional music package"
	
	${NSD_CreateLabel} 0 0 100% 24u "Do you want to download and install an additional music package by David Oerther?"
	Pop $Label

	${NSD_CreateCheckbox} 0 24u 100% 10u "&Download and install music (130MB)"
	Pop $MusicCheckbox
	
	${NSD_CreateLabel} 0 48u 100% 24u "License: Music by David Oerther © Copyright 2015. All rights reserved, note that the soundtrack does not use the creative common or similar licenses."
	
	${NSD_CreateLabel} 0 72u 100% 36u "The game Open Clonk (http://www.openclonk.org/) and all derived works may use and distribute the soundtrack, as long as credit is given. Open Clonk modifications and ingame-videos don't require credits."

	${NSD_CreateLabel} 0 108u 100% 24u "For all other usage (f.e. commercial) please contact: david.oerther@directbox.com"

	${If} $MusicCheckbox_State == ${BST_CHECKED}
		${NSD_Check} $MusicCheckbox
	${EndIf}

	nsDialogs::Show

FunctionEnd

Function nsMusicPageDone
	${NSD_GetState} $MusicCheckbox $MusicCheckbox_State
FunctionEnd

Function MusicPackDownload
	
	; Skip download if user didn't tick the check box
	${If} $MusicCheckbox_State != ${BST_CHECKED}
		StrCpy $TempMusicFn "none"
		DetailPrint "Extra music pack installation skipped."
		Goto musicdldone
	${EndIf}
	
	DetailPrint "Downloading extra music pack from ${MUSICPACK_URL}..."
	
	; Use a nice temp file name so user knows what is being downloaded.
	; GetTempFileName $TempMusicFn
	StrCpy $TempMusicFn "$TEMP\OpenClonkSoundtrack"
	
	; Inetc plugin has better dialogs and progress bars, but the license is unclear
	; inetc::get /caption "Music package download" /popup "" "${MUSICPACK_URL}" "$TempMusicFn" /end
	; Download using builtin NSISdl plugin
	NSISdl::download "${MUSICPACK_URL}" "$TempMusicFn"
	Pop $0
	StrCmp $0 "success" musicdlok
	StrCpy $TempMusicFn "none"
	DetailPrint "Download error: $0"
	MessageBox MB_OK|MB_ICONEXCLAMATION "Download error $0. You can also get the music pack directly from the OpenClonk website." /SD IDOK
	Goto musicdldone
musicdlok:
	DetailPrint "Music pack download done."
musicdldone:
FunctionEnd

; Install music pack if it had been downloaded
Function MusicPackInstall
	; Check successful download
	StrCmp $TempMusicFn "none" musicinstdone
	; Install pack using copy+delete
	DetailPrint "Installing extra music pack..."
	CopyFiles /SILENT "$TempMusicFn" "$INSTDIR\Music.ocg"
	Delete "$TempMusicFn"
musicinstdone:
FunctionEnd
