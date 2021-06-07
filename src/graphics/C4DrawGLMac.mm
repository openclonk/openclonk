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

#define GL_SILENCE_DEPRECATION
#include <epoxy/gl.h>
#include "C4Include.h"
#include "game/C4GraphicsSystem.h"
#include "gui/C4MouseControl.h"
#include "gui/C4Gui.h"
#include "game/C4Game.h"
#include "game/C4Viewport.h"
#include "editor/C4ViewportWindow.h"
#include "editor/C4Console.h"
#include "game/C4FullScreen.h"
#include "player/C4PlayerList.h"
#include "gui/C4Gui.h"
#include "landscape/C4Landscape.h"

#include "graphics/C4DrawGL.h"

#import "graphics/C4DrawGLMac.h"
#import "platform/C4WindowController.h"
#import "platform/C4AppDelegate+MainMenuActions.h"

#ifdef USE_COCOA

@implementation C4OpenGLView

@synthesize context;

- (BOOL) isOpaque {return YES;}


- (id) initWithFrame:(NSRect)frameRect
{
	self = [super initWithFrame:frameRect];
	if (self != nil) {
		[[NSNotificationCenter defaultCenter]
			addObserver:self
			selector:@selector(_surfaceNeedsUpdate:)
			name:NSViewGlobalFrameDidChangeNotification
			object:self];
	}
	return self;
}

- (void) awakeFromNib
{
	[self enableEvents];
}

- (void) enableEvents
{
	[[self window] makeFirstResponder:self];
	[[self window] setAcceptsMouseMovedEvents:YES];
	
	[self registerForDraggedTypes:[NSArray arrayWithObjects:NSFilenamesPboardType, nil]];
}

- (BOOL) acceptsFirstResponder {return YES;}

- (void) resetCursorRects
{
    [super resetCursorRects];
	if ([self shouldHideMouseCursor])
	{
		static NSCursor* cursor;
		if (!cursor)
		{
			cursor = [[NSCursor alloc] initWithImage:[[NSImage alloc] initWithSize:NSMakeSize(1, 1)] hotSpot:NSMakePoint(0, 0)];
		}
		[self addCursorRect:self.bounds cursor:cursor];
	}
}

- (void) _surfaceNeedsUpdate:(NSNotification*)notification
{
   [self update];
}

- (void) lockFocus
{
	NSOpenGLContext* ctx = [self context];
	[super lockFocus];
	if ([ctx view] != self) {
		[ctx setView:self];
    }
    [ctx makeCurrentContext];
	if (!Application.isEditor)
	{
		/*int swapInterval = 1;
		[ctx setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval]; */
	}
}

- (void) viewDidMoveToWindow
{
	[super viewDidMoveToWindow];
	if ([self window] == nil)
		[context clearDrawable];
}

- (void) drawRect:(NSRect)rect
{
	// better not to draw anything when the game has already finished
	if (Application.fQuitMsgReceived)
		return;

#ifdef __MAC_10_9
	// don't draw if tab-switched away from fullscreen
	if ([NSApp respondsToSelector:@selector(occlusionState)])
	{
		// Mavericks - query app occlusion state
		if (([NSApp occlusionState] & NSApplicationOcclusionStateVisible) == 0)
			return;
	}
#endif
	
	if ([self.controller isFullScreenConsideringLionFullScreen] && ![NSApp isActive])
		return;
	if ([self.window isMiniaturized] || ![self.window isVisible])
		return;

	C4Window* stdWindow = self.controller.stdWindow;
	
	if (stdWindow)
		stdWindow->PerformUpdate();
	else
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

- (C4WindowController*) controller {return (C4WindowController*)[self.window delegate];}

int32_t mouseButtonFromEvent(NSEvent* event, DWORD* modifierFlags)
{
	*modifierFlags = [event modifierFlags]; // should be compatible since MK_* constants mirror the NS* constants
	if ([event modifierFlags] & NSEventModifierFlagCommand)
	{
		// treat cmd and ctrl the same
		*modifierFlags |= NSEventModifierFlagControl;
	}
	switch (event.type)
	{
		case NSEventTypeLeftMouseDown:
			return [event clickCount] > 1 ? C4MC_Button_LeftDouble : C4MC_Button_LeftDown;
		case NSEventTypeLeftMouseUp:
			return C4MC_Button_LeftUp;
		case NSEventTypeRightMouseDown:
			return [event clickCount] > 1 ? C4MC_Button_RightDouble : C4MC_Button_RightDown;
		case NSEventTypeRightMouseUp:
			return C4MC_Button_RightUp;
		case NSEventTypeLeftMouseDragged: case NSEventTypeRightMouseDragged:
			return C4MC_Button_None; // sending mouse downs all the time when dragging is not the right thing to do
		case NSEventTypeOtherMouseDown:
			return C4MC_Button_MiddleDown;
		case NSEventTypeOtherMouseUp:
			return C4MC_Button_MiddleUp;
		case NSEventTypeScrollWheel:
			return C4MC_Button_Wheel;
		default:
			break;
	}
	return C4MC_Button_None;
}

- (BOOL) shouldHideMouseCursor
{
	return !Application.isEditor || (self.controller.viewport && Console.EditCursor.GetMode() == C4CNS_ModePlay && ValidPlr(self.controller.viewport->GetPlayer()));
}

- (void) showCursor
{
	if ([self shouldHideMouseCursor])
		[NSCursor unhide];
}

- (void) hideCursor
{
	if ([self shouldHideMouseCursor])
		[NSCursor hide];
}

- (void) mouseEvent:(NSEvent*)event
{
	DWORD flags = 0;
	int32_t button = mouseButtonFromEvent(event, &flags);
	int actualSizeX = Application.isEditor ? self.frame.size.width  : ActualFullscreenX;
	int actualSizeY = Application.isEditor ? self.frame.size.height : ActualFullscreenY;
	CGPoint mouse = [self convertPoint:[self.window mouseLocationOutsideOfEventStream] fromView:nil];
	if (!Application.isEditor)
	{
		mouse.x *= ActualFullscreenX/self.frame.size.width;
		mouse.y *= ActualFullscreenY/self.frame.size.height;
	}
	mouse.x = fmin(fmax(mouse.x, 0), actualSizeX);
	mouse.y = fmin(fmax(mouse.y, 0), actualSizeY);
	int x = mouse.x;
	int y = actualSizeY - mouse.y;

	C4Viewport* viewport = self.controller.viewport;
	if (::MouseControl.IsViewport(viewport) && Console.EditCursor.GetMode() == C4CNS_ModePlay)
	{	
		DWORD keyMask = flags;
		if ([event type] == NSEventTypeScrollWheel)
		{
			// TODO: We could evaluate the full smooth scrolling
			// information, but zoom and inventory scrolling don't
			// do very well with that at the moment.
			if ([event deltaY] > 0)
				keyMask |= (+32) << 16;
			else
				keyMask |= (-32) << 16;
		}

		::C4GUI::MouseMove(button, x, y, keyMask, Application.isEditor ? viewport : NULL);
	}
	else if (viewport)
	{
		switch (button)
		{
		case C4MC_Button_LeftDown:
			Console.EditCursor.Move(viewport->GetViewX()+x/viewport->GetZoom(), viewport->GetViewY()+y/viewport->GetZoom(), viewport->GetZoom(), flags);
			Console.EditCursor.LeftButtonDown(flags);
			break;
		case C4MC_Button_LeftUp:
			Console.EditCursor.LeftButtonUp(flags);
			break;
		case C4MC_Button_RightDown:
			Console.EditCursor.RightButtonDown(flags);
			break;
		case C4MC_Button_RightUp:
			Console.EditCursor.RightButtonUp(flags);
			break;
		case C4MC_Button_None:
			Console.EditCursor.Move(viewport->GetViewX()+x/viewport->GetZoom(),viewport->GetViewY()+y/viewport->GetZoom(), viewport->GetZoom(), flags);
			break;
		}
	}

}

- (void) magnifyWithEvent:(NSEvent *)event
{
	if (Game.IsRunning && (event.modifierFlags & NSEventModifierFlagOption) == 0)
	{
		C4Viewport* viewport = self.controller.viewport;
		if (viewport)
		{
			viewport->SetZoom(viewport->GetZoom()+[event magnification], true);
		}
	}
	else
	{
		if (lionAndBeyond())
			[self.window toggleFullScreen:self];
		else
			[self.controller setFullscreen:[event magnification] > 0];
	}

}

- (void) swipeWithEvent:(NSEvent*)event
{
	// swiping left triggers going back in startup dialogs
	if (event.deltaX > 0)
		[C4AppDelegate.instance simulateKeyPressed:K_LEFT];
	else
		[C4AppDelegate.instance simulateKeyPressed:K_RIGHT];
}

- (void)insertText:(id)insertString
{
	if (::pGUI)
	{
		NSString* str = [insertString isKindOfClass:[NSAttributedString class]] ? [(NSAttributedString*)insertString string] : (NSString*)insertString;
		const char* cstr = [str cStringUsingEncoding:NSUTF8StringEncoding];
		::pGUI->CharIn(cstr);
	}
}

- (void)doCommandBySelector:(SEL)selector
{
	// ignore to not trigger the annoying beep sound
}

- (void)keyEvent:(NSEvent*)event withKeyEventType:(C4KeyEventType)type
{
	Game.DoKeyboardInput(
		[event keyCode]+CocoaKeycodeOffset, // offset keycode by some value to distinguish between those special key defines
		type,
		[event modifierFlags] & NSEventModifierFlagOption,
		([event modifierFlags] & NSEventModifierFlagControl) || ([event modifierFlags] & NSEventModifierFlagCommand),
		[event modifierFlags] & NSEventModifierFlagShift,
		false, NULL
	);

	C4Window* stdWindow = self.controller.stdWindow;
	if (stdWindow->eKind == C4ConsoleGUI::W_Viewport)
	{
		if (type == KEYEV_Down)
			Console.EditCursor.KeyDown([event keyCode]+CocoaKeycodeOffset, [event modifierFlags]);
		else
			Console.EditCursor.KeyUp([event keyCode]+CocoaKeycodeOffset, [event modifierFlags]);
	}
}

- (void)keyDown:(NSEvent*)event
{
	[self interpretKeyEvents:[NSArray arrayWithObject:event]]; // call this to route character input to insertText:
	[self keyEvent:event withKeyEventType:KEYEV_Down];
}

- (void)keyUp:(NSEvent*)event
{
	[self keyEvent:event withKeyEventType:KEYEV_Up];
}

- (void)flagsChanged:(NSEvent*)event
{
	// Send keypress/release events for relevant modifier keys
	// keyDown() is not called for modifier keys.
	C4KeyCode key = (C4KeyCode)([event keyCode] + CocoaKeycodeOffset);
	int modifier = 0;
	if (key == K_SHIFT_L || key == K_SHIFT_R)
		modifier = NSEventModifierFlagShift;
	if (key == K_CONTROL_L || key == K_CONTROL_R)
		modifier = NSEventModifierFlagControl;
	if (key == K_COMMAND_L || key == K_COMMAND_R)
		modifier = NSEventModifierFlagCommand;
	if (key == K_ALT_L || key == K_ALT_R)
		modifier = NSEventModifierFlagOption;

	if (modifier != 0)
	{
		int modifierMask = [event modifierFlags];
		if (modifierMask & modifier)
			[self keyEvent:event withKeyEventType:KEYEV_Down];
		else
			[self keyEvent:event withKeyEventType:KEYEV_Up];
	}
}

- (NSDragOperation) draggingEntered:(id<NSDraggingInfo>)sender
{
	return NSDragOperationCopy;
}

- (NSDragOperation) draggingUpdated:(id<NSDraggingInfo>)sender
{
	return NSDragOperationCopy;
}

- (BOOL) prepareForDragOperation:(id<NSDraggingInfo>)sender
{
	return YES;
}

- (BOOL) performDragOperation:(id<NSDraggingInfo>)sender
{
	NSPasteboard* pasteboard = [sender draggingPasteboard];
	if ([[pasteboard types] containsObject:NSFilenamesPboardType])
	{
		NSArray* files = [pasteboard propertyListForType:NSFilenamesPboardType];
		C4Viewport* viewport = self.controller.viewport;
		if (viewport)
		{
			for (NSString* fileName in files)
			{
				auto loc = NSMakePoint([sender draggingLocation].x, self.bounds.size.height - [sender draggingLocation].y);
				viewport->DropFile([fileName cStringUsingEncoding:NSUTF8StringEncoding], loc.x, loc.y);
			}
		}
	}
	return YES;
}

- (void )concludeDragOperation:(id<NSDraggingInfo>)sender
{
}

// respaghettisize :D

- (void) scrollWheel:(NSEvent *)event
{
	// Scroll viewport in editor mode
	C4Viewport* viewport = self.controller.viewport;
	if (Application.isEditor && viewport && !viewport->GetPlayerLock())
	{
		NSScrollView* scrollView = self.controller.scrollView;
		NSPoint p = NSMakePoint(2*-[event deltaX]/abs(::Landscape.GetWidth()-viewport->ViewWdt), 2*-[event deltaY]/abs(::Landscape.GetHeight()-viewport->ViewHgt));
		[scrollView.horizontalScroller setDoubleValue:scrollView.horizontalScroller.doubleValue+p.x];
		[scrollView.verticalScroller setDoubleValue:scrollView.verticalScroller.doubleValue+p.y];
		viewport->ViewPositionByScrollBars();
		[self display];
	}
	else
	{
		// If player lock is enabled or fullscreen: handle scroll
		// event in-game.
		[self mouseEvent:event];
	}
}

- (void) mouseDown:        (NSEvent *)event {[self mouseEvent:event];}
- (void) rightMouseDown:   (NSEvent *)event {[self mouseEvent:event];}
- (void) rightMouseUp:     (NSEvent *)event {[self mouseEvent:event];}
- (void) otherMouseDown:   (NSEvent *)event {[self mouseEvent:event];}
- (void) otherMouseUp:     (NSEvent *)event {[self mouseEvent:event];}
- (void) mouseUp:          (NSEvent *)event {[self mouseEvent:event];}
- (void) mouseMoved:       (NSEvent *)event {[self mouseEvent:event];}
- (void) mouseDragged:     (NSEvent *)event {[self mouseEvent:event];}
- (void) rightMouseDragged:(NSEvent *)event {[self mouseEvent:event];}

- (void) update
{
	[context update];
}

+ (CGDirectDisplayID) displayID
{
	return (CGDirectDisplayID)[[[[[NSApp keyWindow] screen] deviceDescription] valueForKey:@"NSScreenNumber"] intValue];
}

+ (void) enumerateMultiSamples:(std::vector<int>&)samples
{
	CGDirectDisplayID displayID = self.displayID;
	CGOpenGLDisplayMask displayMask = CGDisplayIDToOpenGLDisplayMask(displayID);
	int numRenderers = 0;
	CGLRendererInfoObj obj = NULL;
	GLint sampleModes;
	
	CGLQueryRendererInfo(displayMask, &obj, &numRenderers);
	CGLDescribeRenderer(obj, 0, kCGLRPSampleModes, &sampleModes);
	CGLDestroyRendererInfo(obj);
	
	if (sampleModes & kCGLMultisampleBit)
	{
		samples.push_back(1);
		samples.push_back(2);
		samples.push_back(4);
	}
}

+ (void) setSurfaceBackingSizeOf:(NSOpenGLContext*) context width:(int)wdt height:(int)hgt
{
	if (context && !Application.isEditor)
	{
		// Make back buffer size fixed ( http://developer.apple.com/library/mac/#documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_contexts/opengl_contexts.html )
		GLint dim[2] = {wdt, hgt};
		CGLContextObj ctx = (CGLContextObj)context.CGLContextObj;
		CGLSetParameter(ctx, kCGLCPSurfaceBackingSize, dim);
		CGLEnable (ctx, kCGLCESurfaceBackingSize);	
	}
}

- (void) setContextSurfaceBackingSizeToOwnDimensions
{
	[C4OpenGLView setSurfaceBackingSizeOf:self.context width:self.frame.size.width height:self.frame.size.height];
}

static NSOpenGLContext* MainContext;

+ (NSOpenGLContext*) mainContext
{
	return MainContext;
}

+ (NSOpenGLContext*) createContext:(CStdGLCtx*) pMainCtx
{
	std::vector<NSOpenGLPixelFormatAttribute> attribs;
	attribs.push_back(NSOpenGLPFAOpenGLProfile);
	attribs.push_back(NSOpenGLProfileVersion3_2Core);
	attribs.push_back(NSOpenGLPFADepthSize);
	attribs.push_back(16);
	if (!Application.isEditor && Config.Graphics.MultiSampling > 0)
	{
		std::vector<int> samples;
		[self enumerateMultiSamples:samples];
		if (!samples.empty())
		{
			attribs.push_back(NSOpenGLPFAMultisample);
			attribs.push_back(NSOpenGLPFASampleBuffers);
			attribs.push_back(1);
			attribs.push_back(NSOpenGLPFASamples);
			attribs.push_back(Config.Graphics.MultiSampling);
		}
	}
	attribs.push_back(NSOpenGLPFANoRecovery);
	//attribs.push_back(NSOpenGLPFADoubleBuffer);
	//attribs.push_back(NSOpenGLPFAWindow); // cannot create a core profile with this
	attribs.push_back(0);
	NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes:&attribs[0]];
	NSOpenGLContext* result = [[NSOpenGLContext alloc] initWithFormat:format shareContext:pMainCtx ? pMainCtx->objectiveCObject<NSOpenGLContext>() : nil];
	if (!MainContext)
		MainContext = result;
	return result;
}

- (IBAction) increaseZoom:(id)sender
{
	if (Application.isEditor)
	{
		C4Viewport* v = self.controller.viewport;
		v->SetZoom(v->GetZoom()*2, false);
	}
	else
		Viewports.ViewportZoomIn();
}

- (IBAction) decreaseZoom:(id)sender
{
	if (Application.isEditor)
	{
		C4Viewport* v = self.controller.viewport;
		v->SetZoom(v->GetZoom()/2, false);
	}
	else
		Viewports.ViewportZoomOut();
}

@end

@implementation C4EditorOpenGLView

- (void) copy:(id) sender
{
	Console.EditCursor.Duplicate();
}

- (void) delete:(id) sender
{
	Console.EditCursor.Delete();
}

- (IBAction) grabContents:(id) sender
{
	Console.EditCursor.GrabContents();
}

- (IBAction) resetZoom:(id) sender
{
	self.controller.viewport->SetZoom(1, true);
}

@end

#pragma mark CStdGLCtx: Initialization

CStdGLCtx::CStdGLCtx(): pWindow(0), this_context(contexts.end()) {}

void CStdGLCtx::Clear(bool multisample_change)
{
	Deselect();
	setObjectiveCObject(nil);
	pWindow = 0;

	if (this_context != contexts.end())
	{
		contexts.erase(this_context);
		this_context = contexts.end();
	}
}

void C4Window::EnumerateMultiSamples(std::vector<int>& samples, int) const
{
	[C4OpenGLView enumerateMultiSamples:samples];
}

bool CStdGLCtx::Init(C4Window * pWindow, C4AbstractApp *)
{
	// safety
	if (!pGL) return false;
	// store window
	this->pWindow = pWindow;
	// Create Context with sharing (if this is the main context, our ctx will be 0, so no sharing)
	// try direct rendering first
	NSOpenGLContext* ctx = [C4OpenGLView createContext:pGL->pMainCtx];
	setObjectiveCObject(ctx);
	// No luck at all?
	if (!Select(true)) return pGL->Error("  gl: Unable to select context");
	// set the openglview's context
	auto controller = pWindow->objectiveCObject<C4WindowController>();
	if (controller && controller.openGLView)
	{
		[controller.openGLView setContext:ctx];
		[ctx setView:controller.openGLView];
	}

	this_context = contexts.insert(contexts.end(), this);
	return true;
}

#pragma mark CStdGLCtx: Select/Deselect

bool CStdGLCtx::Select(bool verbose)
{
	[objectiveCObject<NSOpenGLContext>() makeCurrentContext];
	SelectCommon();
	// update clipper - might have been done by UpdateSize
	// however, the wrong size might have been assumed
	if (!pGL->UpdateClipper())
	{
		if (verbose) pGL->Error("  gl: UpdateClipper failed");
		return false;
	}
	// success
	return true;
}

void CStdGLCtx::Deselect()
{
	if (pGL && pGL->pCurrCtx == this)
    {
		pGL->pCurrCtx = 0;
    }
}

bool CStdGLCtx::PageFlip()
{
	// flush GL buffer
	glFlush();
	if (!pWindow) return false;
	//SDL_GL_SwapBuffers();
	return true;
}

int ActualFullscreenX = 0, ActualFullscreenY = 0;

#endif
