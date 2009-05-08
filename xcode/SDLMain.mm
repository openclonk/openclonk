// Roughly adapted from the original SDLMain.m; haxxed to death by teh Gurkendoktor.
// Look at main() to get an idea for what happens here.

#import <Carbon/Carbon.h>
#import "SDLMain.h"
#import "SDL.h"

#import <sys/param.h> /* for MAXPATHLEN */
#import <unistd.h>
#include <vector>
#include <string>
#include <cstring>
using namespace std;

// Including Std* leads to linker trouble. FUN!
bool GetParentPath(const char *szFilename, char *szBuffer);
const char* GetFilename(const char *fname);
const char *GetExtension(const char *fname);
bool CopyFile(const char *szSource, const char *szTarget, bool FailIfExists);
bool SEqualNoCase(const char *szStr1, const char *szStr2, int iLen=-1);
void SAppendChar(char cChar, char *szTarget);
void SAppend(const char *szSource, char *szTarget, int iMaxL=-1);

namespace {
    const string& getClonkDir() {
        static string result;
        if (result.empty()) {
            // Where is the .app bundle?
            char path[PATH_MAX];
            char pathTmp[PATH_MAX];
            NSBundle* bundle = [NSBundle mainBundle];
            if (bundle &&
                GetParentPath([[bundle resourcePath] UTF8String], path) &&
                GetParentPath(path, pathTmp) &&
                GetParentPath(pathTmp, path))
                result = path;
        }
        return result;
    }

    vector<char*> soy_argv; // faked argv collected during openFile calls
    BOOL gDoNotLaunch = NO;
    BOOL gHasFinished = NO;
}

/* The main class of the application, the application's delegate */
@implementation SDLMain

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
    const char* szFilename = [filename UTF8String];
    const char* szExt = GetExtension(szFilename);

    if (SEqualNoCase(szExt, "c4k") || SEqualNoCase(szExt, "c4u"))
    {
        // Key/Update files: Just pass to engine, will install
        soy_argv.push_back(strdup(szFilename));
        fflush(0);
        return YES;
    }

    if (SEqualNoCase(szExt, "c4d") || SEqualNoCase(szExt, "c4s") || SEqualNoCase(szExt, "c4f") ||
        SEqualNoCase(szExt, "c4g") || SEqualNoCase(szExt, "c4s"))
    {
        gDoNotLaunch = YES;

        // Build destination path.
        char destPath[PATH_MAX] = { 0 };
        SAppend(getClonkDir().c_str(), destPath);
        SAppendChar('/', destPath);
        SAppend(GetFilename(szFilename), destPath);

        NSString* formatString;

        // Already installed?
        if (SEqualNoCase(szFilename, destPath))
        {
            formatString = NSLocalizedString(@"AddOnIsAlreadyInstalled", nil);
            NSRunInformationalAlertPanel(NSLocalizedString(@"AddOnInstallationTitle", nil),
                [NSString stringWithFormat: formatString, GetFilename(szFilename)],
                @"OK", nil, nil);
            return NO;
        }

        // Success?
        if(CopyFile(szFilename, destPath, false))
            formatString = NSLocalizedString(@"AddOnInstallationSuccess", nil);
        else
            formatString = NSLocalizedString(@"AddOnInstallationFailure", nil);

        NSRunInformationalAlertPanel(NSLocalizedString(@"AddOnInstallationTitle", nil),
            [NSString stringWithFormat: formatString, GetFilename(szFilename)],
            @"OK", nil, nil);
        return YES;
    }

    return NO;
}

- (void)getUrl:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
	NSString *url = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];
    soy_argv.push_back(strdup([url cString]));
}

/* Called when the internal event loop has just started running */
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
    gHasFinished = YES;
    fflush(0);
}

- (void)terminate:(id)sender
{
    // Post an SDL_QUIT event
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}
@end

#ifdef main
#  undef main
#endif

/* Main entry point to executable - should *not* be SDL_main! */
int main (int argc, char **argv)
{
    // Catches whatever would be leaked otherwise.
    // Gets rid of warnings from hax0red SDL framework.
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    chdir(getClonkDir().c_str());

    // Global, temporary ARGV. Will be appended to by openFile.
    for (int i = 0; i < argc; ++i)
        soy_argv.push_back(argv[i]);

    // This is passed if we are launched by double-clicking
    // Get rid of it.
    if ( argc >= 2 && strncmp (argv[1], "-psn", 4) == 0 )
        soy_argv.erase(soy_argv.begin() + 1);

    // Set up application & delegate.
    [NSApplication sharedApplication];
    SDLMain *sdlMain = [[SDLMain alloc] init];
    [NSApp setDelegate:sdlMain];
	[[NSAppleEventManager sharedAppleEventManager] setEventHandler:sdlMain
        andSelector:@selector(getUrl:withReplyEvent:) forEventClass:kInternetEventClass andEventID:kAEGetURL];
    [NSBundle loadNibNamed:@"SDLMain" owner:NSApp];
    [NSApp finishLaunching];

    // Now there's openFile calls and all that waiting for us in the event queue.
    // Fetch events until applicationHasFinishedLaunching was called.
    while (!gHasFinished) {
        NSEvent *event = [NSApp nextEventMatchingMask:NSAnyEventMask
                                untilDate:nil
                                inMode:NSDefaultRunLoopMode
                                dequeue:YES ];
        if ( event == nil ) {
            break;
        }
        [NSApp sendEvent:event];
    }

    // Should we run the Clonk engine?
    // Some file types may set this to false, ie. when installing add-ons.
    if (gDoNotLaunch)
        return 0;

    // Hand off to Clonk code
    vector<char*> argv_copy(soy_argv.begin(), soy_argv.end());
    argv_copy.push_back(0);
    fflush(0);
    int status = SDL_main(argv_copy.size() - 1, &argv_copy[0]);

    return status;
}
