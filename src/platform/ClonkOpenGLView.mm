/*
 * OpenClonk, http://www.openclonk.org
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

#include <C4Include.h>
#include <C4GraphicsSystem.h>
#include <C4MouseControl.h>
#include <C4GUI.h>
#include <C4Game.h>
#include <C4Viewport.h>
#include <C4ViewportWindow.h>
#include <C4Console.h>
#include <C4Fullscreen.h>
#include <C4PlayerList.h>
#include <C4Gui.h>
#include <C4Landscape.h>

#include <StdGL.h>

#import "ClonkOpenGLView.h"
#import "ClonkWindowController.h"
#import "ClonkMainMenuActions.h"

#ifdef USE_COCOA

@implementation ClonkOpenGLView

@synthesize context;

- (BOOL) isOpaque {return YES;}
- (NSOpenGLContext*) context {return context;}

- (void) dealloc
{
	[context release];
	[super dealloc];
}

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
	// don't draw if tab-switched away from fullscreen
	if ([self.controller isFullscreen] && ![NSApp isActive]) // ugh - no way to find out if window is hidden due hidesondeactivate?
		return;
	if ([self.window isMiniaturized] || ![self.window isVisible])
		return;
	//[self.context update];
	CStdWindow* stdWindow = self.controller.stdWindow;
	
	if (stdWindow)
		stdWindow->PerformUpdate();
	else
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

- (ClonkWindowController*) controller {return (ClonkWindowController*)[self.window delegate];}

int32_t mouseButtonFromEvent(NSEvent* event, DWORD& modifierFlags)
{
	modifierFlags = [event modifierFlags]; // should be compatible since MK_* constants mirror the NS* constants
	if ([event modifierFlags] & NSCommandKeyMask)
	{
		// treat cmd and ctrl the same
		modifierFlags |= NSControlKeyMask;
	}
	switch (event.type)
	{
		case NSLeftMouseDown:
			return [event clickCount] > 1 ? C4MC_Button_LeftDouble : C4MC_Button_LeftDown;
		case NSLeftMouseUp:
			return C4MC_Button_LeftUp;
		case NSRightMouseDown:
			return [event clickCount] > 1 ? C4MC_Button_RightDouble : C4MC_Button_RightDown;
		case NSRightMouseUp:
			return C4MC_Button_RightUp;
		case NSLeftMouseDragged: case NSRightMouseDragged:
			return C4MC_Button_None; // sending mouse downs all the time when dragging is not the right thing to do
		case NSOtherMouseDown:
			return C4MC_Button_MiddleDown;
		case NSOtherMouseUp:
			return C4MC_Button_MiddleUp;
		case NSScrollWheel:
			return C4MC_Button_Wheel;
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
	int32_t button = mouseButtonFromEvent(event, flags);
	NSPoint point = [self convertPoint:[self.window mouseLocationOutsideOfEventStream] fromView:nil];
	int actualSizeX = Config.Graphics.ResX;
	int actualSizeY = Config.Graphics.ResY;
	if (!Application.isEditor)
	{
		point.x *= Config.Graphics.ResX/[self bounds].size.width;
		point.y *= Config.Graphics.ResY/[self bounds].size.height;
	}
	else
	{
		actualSizeX = self.frame.size.width;
		actualSizeY = self.frame.size.height;
	}
	int x = fmin(fmax(point.x, 0), actualSizeX);
	int y = fmin(fmax(actualSizeY - point.y, 0), actualSizeY);
	
	{
		C4Viewport* viewport = self.controller.viewport;
		if (::MouseControl.IsViewport(viewport) && Console.EditCursor.GetMode() == C4CNS_ModePlay)
		{
			::C4GUI::MouseMove(button, x, y, [event type] == NSScrollWheel ? ((int)[event deltaY] << 16) : 0, viewport);
		}
		else if (viewport)
		{
			switch (button)
			{
			case C4MC_Button_LeftDown:
				Console.EditCursor.Move(viewport->ViewX+x/viewport->GetZoom(), viewport->ViewY+y/viewport->GetZoom(), 0);
				Console.EditCursor.LeftButtonDown(!!(flags & MK_CONTROL));
				break;
			case C4MC_Button_LeftUp:
				Console.EditCursor.LeftButtonUp();
				break;
			case C4MC_Button_RightDown:
				Console.EditCursor.RightButtonDown(!!(flags & MK_CONTROL));
				break;
			case C4MC_Button_RightUp:
				Console.EditCursor.RightButtonUp();
				break;
			case C4MC_Button_None:
				Console.EditCursor.Move(viewport->ViewX+x/viewport->GetZoom(),viewport->ViewY+y/viewport->GetZoom(), 0);
				break;
			}
		}
	}

}

- (void) magnifyWithEvent:(NSEvent *)event
{
	if (Game.IsRunning)
	{
		C4Viewport* viewport = self.controller.viewport;
		if (viewport)
		{
			viewport->SetZoom(viewport->GetZoom()+[event magnification], true);
		}
	}
	else
	{
		[self.controller setFullscreen:[event magnification] > 0];
	}

}

- (void) swipeWithEvent:(NSEvent*)event
{
	// swiping left triggers going back in startup dialogs
	if (event.deltaX > 0)
		[ClonkAppDelegate.instance simulateKeyPressed:K_LEFT];
	else
		[ClonkAppDelegate.instance simulateKeyPressed:K_RIGHT];
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
		[event modifierFlags] & NSAlternateKeyMask,
		([event modifierFlags] & NSControlKeyMask) || ([event modifierFlags] & NSCommandKeyMask),
		[event modifierFlags] & NSShiftKeyMask,
		false, NULL
	);
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
				viewport->DropFile([fileName cStringUsingEncoding:NSUTF8StringEncoding], [sender draggingLocation].x, [sender draggingLocation].y);
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
	if (!Application.isEditor)
		[self mouseEvent:event];
	else
	{
		C4Viewport* viewport = self.controller.viewport;
		if (viewport)
		{
			NSScrollView* scrollView = self.controller.scrollView;
			NSPoint p = NSMakePoint(2*-[event deltaX]/abs(GBackWdt-viewport->ViewWdt), 2*-[event deltaY]/abs(GBackHgt-viewport->ViewHgt));
			[scrollView.horizontalScroller setDoubleValue:scrollView.horizontalScroller.doubleValue+p.x];
			[scrollView.verticalScroller setDoubleValue:scrollView.verticalScroller.doubleValue+p.y];
			viewport->ViewPositionByScrollBars();
			[self display];
		}
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
	[ClonkOpenGLView setSurfaceBackingSizeOf:self.context width:self.frame.size.width height:self.frame.size.height];
}

static NSOpenGLContext* MainContext;

+ (NSOpenGLContext*) mainContext
{
	return MainContext;
}

+ (NSOpenGLContext*) createContext:(CStdGLCtx*) pMainCtx
{
	std::vector<NSOpenGLPixelFormatAttribute> attribs;
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
	attribs.push_back(NSOpenGLPFAWindow);
	attribs.push_back(0);
	NSOpenGLPixelFormat* format = [[[NSOpenGLPixelFormat alloc] initWithAttributes:&attribs[0]] autorelease];

	NSOpenGLContext* result = [[NSOpenGLContext alloc] initWithFormat:format shareContext:pMainCtx ? (NSOpenGLContext*)pMainCtx->GetNativeCtx() : nil];
	[self setSurfaceBackingSizeOf:result width:Config.Graphics.ResX height:Config.Graphics.ResY];
	if (!MainContext)
	{
		MainContext = result;
	}
	return result;
}

@end

@implementation ClonkEditorOpenGLView

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

- (IBAction) increaseZoom:(id)sender
{
	C4Viewport* v = self.controller.viewport;
	v->SetZoom(v->GetZoom()*2, false);
}

- (IBAction) decreaseZoom:(id)sender
{
	C4Viewport* v = self.controller.viewport;
	v->SetZoom(v->GetZoom()/2, false);
}

@end

#pragma mark CStdGLCtx: Initialization

void* CStdGLCtx::GetNativeCtx()
{
	return ctx;
}

CStdGLCtx::CStdGLCtx(): pWindow(0), ctx(nil) {}

void CStdGLCtx::Clear()
{
	Deselect();
	if (ctx)
	{
		[(NSOpenGLContext*)ctx release];
		ctx = nil;
	}
	pWindow = 0;
}

void CStdWindow::EnumerateMultiSamples(std::vector<int>& samples) const
{
	[ClonkOpenGLView enumerateMultiSamples:samples];
}

bool CStdApp::SetVideoMode(unsigned int iXRes, unsigned int iYRes, unsigned int iColorDepth, unsigned int iRefreshRate, unsigned int iMonitor, bool fFullScreen)
{
	ClonkWindowController* controller = (ClonkWindowController*)pWindow->GetController();
	NSWindow* window = controller.window;

	pWindow->SetSize(iXRes, iYRes);
	[controller setFullscreen:fFullScreen];
	[window setAspectRatio:[[window contentView] frame].size];
	[window center];
	if (!fFullScreen)
		[window makeKeyAndOrderFront:nil];
	OnResolutionChanged(iXRes, iYRes);
	return true;
}

bool CStdGLCtx::Init(CStdWindow * pWindow, CStdApp *)
{
	// safety
	if (!pGL) return false;
	// store window
	this->pWindow = pWindow;
	// Create Context with sharing (if this is the main context, our ctx will be 0, so no sharing)
	// try direct rendering first
	NSOpenGLContext* ctx = [ClonkOpenGLView createContext:pGL->pMainCtx];
	this->ctx = (void*)ctx;
	// No luck at all?
	if (!Select(true)) return pGL->Error("  gl: Unable to select context");
	// init extensions
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		// Problem: glewInit failed, something is seriously wrong.
		return pGL->Error(reinterpret_cast<const char*>(glewGetErrorString(err)));
	}
	// set the openglview's context
	ClonkWindowController* controller = (ClonkWindowController*)pWindow->GetController();
	if (controller && controller.openGLView)
	{
		[controller.openGLView setContext:ctx];
	}
	return true;
}

#pragma mark CStdGLCtx: Select/Deselect

bool CStdGLCtx::Select(bool verbose)
{
	if (ctx)
		[(NSOpenGLContext*)ctx makeCurrentContext];
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

#pragma mark CStdGLCtx: Gamma

namespace
{
	class GammaRampConversionTable
	{
	public:
		CGGammaValue red[65535];
		CGGammaValue green[65535];
		CGGammaValue blue[65535];
		GammaRampConversionTable()
		{
			for (int i = 0; i < 65535; i++)
			{
				red[i] = static_cast<float>(i)/65535;
				green[i] = static_cast<float>(i)/65535;
				blue[i] = static_cast<float>(i)/65535;
			}
		}
		static GammaRampConversionTable singleton;
	};
	
	GammaRampConversionTable GammaRampConversionTable::singleton;
}

bool CStdGL::ApplyGammaRamp(_D3DGAMMARAMP& ramp, bool fForce)
{
	CGGammaValue r[256];
	CGGammaValue g[256];
	CGGammaValue b[256];
	for (int i = 0; i < 256; i++)
	{
		r[i] = GammaRampConversionTable::singleton.red[ramp.red[i]];
		g[i] = GammaRampConversionTable::singleton.green[ramp.green[i]];
		b[i] = GammaRampConversionTable::singleton.blue[ramp.blue[i]];
	}
	CGSetDisplayTransferByTable(ClonkOpenGLView.displayID, 256, r, g, b);
	return true;
}

bool CStdGL::SaveDefaultGammaRamp(CStdWindow * pWindow)
{
	return false;//return SDL_GetGammaRamp(DefRamp.ramp.red, DefRamp.ramp.green, DefRamp.ramp.blue) != -1;
}

#endif