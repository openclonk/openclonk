/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

#include <GL/glew.h>

#include "C4Include.h"
#include "editor/C4Console.h"
#include "game/C4Application.h"

#include "control/C4GameSave.h"
#include "game/C4Game.h"
#include "gui/C4MessageInput.h"
#include "C4Version.h"
#include "c4group/C4Language.h"
#include "player/C4Player.h"
#include "landscape/C4Landscape.h"
#include "landscape/C4Sky.h"
#include "game/C4GraphicsSystem.h"
#include "player/C4PlayerList.h"
#include "control/C4GameControl.h"
#include "landscape/C4Texture.h"

#include "platform/StdFile.h"
#include "platform/StdRegistry.h"

#import <Cocoa/Cocoa.h>
#import "platform/C4AppDelegate.h"
#import "editor/C4EditorWindowController.h"
#import "graphics/C4DrawGLMac.h"

// implementation of C4Console GUI for Mac OS X

static inline C4EditorWindowController* ctrler(C4ConsoleGUI* gui) {return gui->objectiveCObject<C4EditorWindowController>();}

class C4ConsoleGUI::State: public C4ConsoleGUI::InternalState<class C4ConsoleGUI>
{
public:
	State(C4ConsoleGUI *console): Super(console) {}
};

class C4ToolsDlg::State: public C4ConsoleGUI::InternalState<class C4ToolsDlg>
{
public:
	State(C4ToolsDlg *toolsDlg): Super(toolsDlg) {}
	~State()
	{
		[ctrler(&Console).toolsPanel orderOut:nil];
	}
	CGImageRef CreatePreviewImage();
	void Default() {}
	void Clear() {}
};

bool C4ConsoleGUI::CreateConsoleWindow(C4AbstractApp *application)
{
	C4WindowController* controller = [C4EditorWindowController new];
	setObjectiveCObject(controller);
	[NSBundle loadNibNamed:@"Editor" owner:controller];
	[controller setStdWindow:this];
	this->Active = true;
	return true;
}

void C4ConsoleGUI::DeleteConsoleWindow()
{
}

void C4ConsoleGUI::Out(const char* message)
{
	C4EditorWindowController* controller = ctrler(this);
	if (controller)
	{
		NSTextStorage* textStorage = controller.outputTextView.textStorage;
		[textStorage appendAttributedString:[[NSAttributedString alloc] initWithString:[NSString stringWithFormat:@"%s\n", message]]];
		[controller.outputTextView scrollRangeToVisible:NSMakeRange([textStorage length]-1, 1)];
	}
}

bool C4ConsoleGUI::ClearLog()
{
	[ctrler(this).outputTextView.textStorage
	replaceCharactersInRange:NSMakeRange(0, ctrler(this).outputTextView.textStorage.length)
	    withAttributedString:[[NSAttributedString alloc] initWithString:@""]];
	return true;
}

void C4ConsoleGUI::DisplayInfoText(C4ConsoleGUI::InfoTextType type, StdStrBuf& text)
{
	NSTextField* label;
	switch (type)
	{
	case CONSOLE_FrameCounter:
		label = ctrler(this).frameLabel;
		break;
	case CONSOLE_TimeFPS:
		label = ctrler(this).timeLabel;
		break;
	case CONSOLE_Cursor:
		return;
	default:
		return;
	}
	[label setStringValue:[NSString stringWithUTF8String:text.getData()]];
}

void C4ConsoleGUI::SetCaptionToFileName(const char* file_name)
{
	[ctrler(this).window setRepresentedFilename:[NSString stringWithUTF8String:file_name]];
}

bool C4ConsoleGUI::FileSelect(StdStrBuf *sFilename, const char * szFilter, DWORD dwFlags, bool fSave)
{
	NSSavePanel* savePanel = fSave ? [NSSavePanel savePanel] : [NSOpenPanel openPanel];
	if (!fSave)
	{
		[(NSOpenPanel*)savePanel setCanChooseFiles:YES];
		[(NSOpenPanel*)savePanel setCanChooseDirectories:YES];
	}
	if ([savePanel runModal] == NSFileHandlingPanelOKButton && [[savePanel URL] isFileURL])
	{
		sFilename->Copy([[savePanel URL].path cStringUsingEncoding:NSUTF8StringEncoding]);
		return true;
	}
	else
		return false;
}

void C4ConsoleGUI::AddMenuItemForPlayer(C4Player* player, StdStrBuf& player_text)
{
	NSMenuItem* item = [
		[C4AppDelegate instance].addViewportForPlayerMenuItem.submenu
		addItemWithTitle:[NSString stringWithUTF8String:player_text.getData()] action:@selector(newViewportForPlayer:) keyEquivalent:@""
	];
	[item setTag:player->Number];
	[item setTarget: C4AppDelegate.instance];
}

void C4ConsoleGUI::ClearViewportMenu()
{
	[[C4AppDelegate instance].addViewportForPlayerMenuItem.submenu removeAllItems];
}

bool C4ConsoleGUI::Message(const char *message, bool query)
{
	NSAlert* alert = [NSAlert
		alertWithMessageText:[NSString stringWithUTF8String:C4ENGINECAPTION]
		defaultButton:query ? @"Yes" : nil
		alternateButton:query ? @"No" : nil
		otherButton:nil
		informativeTextWithFormat:[NSString stringWithUTF8String:message]
	];
	[alert runModal];
	return true;
}

bool C4ConsoleGUI::PropertyDlgOpen()
{
	[ctrler(this).objectsPanel orderFront:nil];
	return true;
}

void C4ConsoleGUI::PropertyDlgClose()
{
	[ctrler(this).objectsPanel orderOut:nil];
}

void C4ConsoleGUI::PropertyDlgUpdate(C4EditCursorSelection &rSelection, bool force_function_update)
{	
	if (![ctrler(this).objectsPanel isVisible])
		return;
	StdStrBuf text = rSelection.GetDataString();
	[ctrler(this).objectPropertiesText.textStorage setAttributedString:[[NSAttributedString alloc] initWithString:[NSString stringWithUTF8String:text.getData()]]];
}

void C4ConsoleGUI::ToolsDlgClose()
{
	[ctrler(this).toolsPanel orderOut:nil];
}

bool C4ConsoleGUI::ToolsDlgOpen(C4ToolsDlg *dlg)
{
	[ctrler(this).toolsPanel orderFront:nil];
	return true;
}

void C4ConsoleGUI::ToolsDlgInitMaterialCtrls(class C4ToolsDlg *dlg)
{
	NSPopUpButton* materialsPopup = ctrler(this).materialsPopup;
	[materialsPopup removeAllItems];
	[materialsPopup addItemWithTitle:[NSString stringWithUTF8String:C4TLS_MatSky]];
	NSMutableArray* ary = [[NSMutableArray alloc] initWithCapacity:MaterialMap.Num+1];
	[ary addObject:[NSString stringWithUTF8String:C4TLS_MatSky]];
	for (int32_t cnt = 0; cnt < ::MaterialMap.Num; cnt++)
	{
		[ary addObject:[NSString stringWithUTF8String: ::MaterialMap.Map[cnt].Name]];
	}
	[ary sortUsingComparator:^(id a, id b) {return [a compare:b];}];
	for (NSString* s in ary)
	{
		[materialsPopup addItemWithTitle:s];
	}
	[materialsPopup selectItemWithTitle:[NSString stringWithUTF8String:dlg->Material]];
}

void C4ToolsDlg::UpdateToolCtrls()
{
	[ctrler(&Console).toolSelector setSelectedSegment:Tool];
}

void C4ToolsDlg::UpdateTextures()
{
	// Refill dlg
	NSPopUpButton* texturesPopup = ctrler(&Console).texturesPopup;
	[texturesPopup removeAllItems];
	// bottom-most: any invalid textures
	bool fAnyEntry = false; int32_t cnt; const char *szTexture;
	if (::Landscape.GetMode()!=LandscapeMode::Exact)
		for (cnt=0; (szTexture=::TextureMap.GetTexture(cnt)); cnt++)
		{
			if (!::TextureMap.GetIndex(Material, szTexture, false))
			{
				fAnyEntry = true;
				[texturesPopup insertItemWithTitle:[NSString stringWithUTF8String:szTexture] atIndex:0];
			}
		}
	// separator
	if (fAnyEntry)
	{
		[texturesPopup insertItemWithTitle:@"-------" atIndex:0];
	}

	// atop: valid textures
	for (cnt=0; (szTexture=::TextureMap.GetTexture(cnt)); cnt++)
	{
		// Current material-texture valid? Always valid for exact mode
		if (::TextureMap.GetIndex(Material,szTexture,false) || ::Landscape.GetMode()==LandscapeMode::Exact)
		{
			[texturesPopup insertItemWithTitle:[NSString stringWithUTF8String:szTexture] atIndex:0];
		}
	}
	// reselect current
	[texturesPopup selectItemWithTitle:[NSString stringWithUTF8String:Texture]];
}

void C4ToolsDlg::NeedPreviewUpdate()
{
	CGImageRef image = state->CreatePreviewImage();
	[ctrler(&Console).previewView setImage:image imageProperties:[NSDictionary dictionary]];
	CGImageRelease(image);
}

namespace
{
	// copy-pasta from http://stackoverflow.com/questions/2395650/fastest-way-to-draw-a-screen-buffer-on-the-iphone
	const void* GetBytePointer(void* info)
	{
		// this is currently only called once
		return info; // info is a pointer to the buffer
	}

	void ReleaseBytePointer(void*info, const void* pointer)
	{
		// don't care, just using the one static buffer at the moment
	}


	size_t GetBytesAtPosition(void* info, void* buffer, off_t position, size_t count)
	{
		// I don't think this ever gets called
		memcpy(buffer, ((char*)info) + position, count);
		return count;
	}
}

CGImageRef C4ToolsDlg::State::CreatePreviewImage()
{
	C4Surface * sfcPreview;
	int32_t iPrvWdt,iPrvHgt;

	iPrvWdt = [ctrler(&Console).previewView frame].size.width;
	iPrvHgt = [ctrler(&Console).previewView frame].size.height;

	if (!(sfcPreview=new C4Surface(iPrvWdt,iPrvHgt,0))) return NULL;

	// fill bg
	BYTE bCol = 0;
	C4Pattern Pattern;
	// Sky material: sky as pattern only
	if (SEqual(GetOwner()->Material,C4TLS_MatSky))
	{
		Pattern.Set(::Landscape.GetSky().Surface, 0);
	}
	// Material-Texture
	else
	{
		bCol=Mat2PixColDefault(::MaterialMap.Get(GetOwner()->Material));
		// Get/Create TexMap entry
		BYTE iTex = ::TextureMap.GetIndex(GetOwner()->Material, GetOwner()->Texture, true);
		if (iTex)
		{
			// Define texture pattern
			const C4TexMapEntry *pTex = ::TextureMap.GetEntry(iTex);
			// Security
			if (pTex)
			{
				// Set drawing pattern
				Pattern = pTex->GetPattern();
			}
		}
	}
	
	pDraw->DrawPatternedCircle(
		sfcPreview,
		iPrvWdt/2,iPrvHgt/2,
		GetOwner()->Grade,
		bCol, Pattern, *::Landscape.GetPal()
	);
	//Application.DDraw->AttachPrimaryPalette(sfcPreview);

	DWORD *pixels = new DWORD[iPrvWdt*iPrvHgt];
	for (int x = 0; x < iPrvWdt; x++) for (int y = 0; y < iPrvHgt; y++)
	{
		pixels[x+y*iPrvWdt] = sfcPreview->GetPixDw(x, y, true);
	}
	CGDataProviderDirectCallbacks callbacks = {0, ::GetBytePointer, ::ReleaseBytePointer, ::GetBytesAtPosition, 0};
	CGDataProviderRef pixelData = CGDataProviderCreateDirect(pixels, iPrvWdt*iPrvHgt*sizeof(DWORD), &callbacks);
	CGImageRef image = CGImageCreate(iPrvWdt, iPrvHgt, 8, 4*8, iPrvWdt*4, [[NSColorSpace deviceRGBColorSpace] CGColorSpace], kCGBitmapByteOrder32Little, pixelData, NULL, true, kCGRenderingIntentDefault);
	CGDataProviderRelease(pixelData);
	delete sfcPreview;
	return image;
}

void C4ToolsDlg::InitGradeCtrl()
{
}

bool C4ToolsDlg::PopMaterial()
{
	return true;
}

void C4ConsoleGUI::ShowAboutWithCopyright(StdStrBuf &copyright)
{
	StdStrBuf strMessage; strMessage.Format("%s %s\n\n%s", C4ENGINECAPTION, C4VERSION, copyright.getData());
	Message(strMessage.getData(), false);
}

void C4ConsoleGUI::ToolsDlgSelectTexture(C4ToolsDlg *dlg, const char *texture)
{
	[ctrler(this).texturesPopup selectItemWithTitle:[NSString stringWithUTF8String:texture]];
}

void C4ConsoleGUI::ToolsDlgSelectMaterial(C4ToolsDlg *dlg, const char *material)
{
	[ctrler(this).materialsPopup selectItemWithTitle:[NSString stringWithUTF8String:material]];
}

void C4ConsoleGUI::ToolsDlgSelectBackTexture(C4ToolsDlg *dlg, const char *texture)
{
	// TODO
}

void C4ConsoleGUI::ToolsDlgSelectBackMaterial(C4ToolsDlg *dlg, const char *material)
{
	// TODO
}

void C4ToolsDlg::UpdateIFTControls()
{
}

void C4ConsoleGUI::SetCursor(C4ConsoleGUI::Cursor cursor)
{
}

void C4ConsoleGUI::RecordingEnabled()
{
	[C4AppDelegate.instance.recordMenuItem setEnabled:NO];
}

void C4ConsoleGUI::AddNetMenu()
{
	[C4AppDelegate.instance.netMenu setHidden:NO];
}

void C4ConsoleGUI::ClearNetMenu()
{
	[C4AppDelegate.instance.netMenu setHidden:YES];
}

void C4ConsoleGUI::DoEnableControls(bool fEnable)
{
	[ctrler(this).modeSelector setEnabled:fEnable];
}

bool C4ConsoleGUI::DoUpdateHaltCtrls(bool fHalt)
{
	return true;
}

void C4ToolsDlg::EnableControls()
{
	NeedPreviewUpdate();
}

void C4ConsoleGUI::ClearPlayerMenu()
{
}

void C4ConsoleGUI::AddNetMenuItemForPlayer(int32_t index, StdStrBuf &text, C4ConsoleGUI::ClientOperation op)
{
	NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithCString:text.getData() encoding:NSUTF8StringEncoding] action:@selector(kickPlayer:) keyEquivalent:[NSString string]];
	[item setTarget:ctrler(this)];
	[C4AppDelegate.instance.netMenu.submenu addItem:item];
}

void C4ConsoleGUI::SetInputFunctions(std::list<const char*> &functions)
{
	[ctrler(this).consoleCombo setStringValue:[NSString string]];
	[ctrler(this) setInputFunctions:functions];
}

void C4ConsoleGUI::AddKickPlayerMenuItem(C4Player *player, StdStrBuf& player_text, bool enabled)
{
	NSMenuItem* item = [
		[C4AppDelegate instance].kickPlayerMenuItem.submenu
		addItemWithTitle:[NSString stringWithUTF8String:player_text.getData()] action:@selector(kickPlayer:) keyEquivalent:@""
	];
	[item setEnabled:enabled];
	[item setTag:player->Number];
	[item setTarget: ctrler(this)];
}

bool C4ToolsDlg::PopTextures()
{
	return true;
}

void C4ToolsDlg::UpdateLandscapeModeCtrls()
{
}

bool C4ConsoleGUI::UpdateModeCtrls(int iMode)
{
	return true;
}

#define CONSOLEGUICOMMONINCLUDE
#include "editor/C4ConsoleGUICommon.h"
