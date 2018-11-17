#import <Foundation/Foundation.h>
#import <Appkit/AppKit.h>

bool osx_set_background(const char *path) {
	NSString *nspath = [[NSString string] initWithUTF8String:path];
	NSURL *url = [NSURL fileURLWithPath:nspath isDirectory:NO];
	// The default options are what we want actually
	NSDictionary *opts = @{};
	bool img_set = true;
	for (NSScreen *screen in [NSScreen screens]) {
		img_set = img_set && [[NSWorkspace sharedWorkspace] setDesktopImageURL:url
								forScreen:screen
								options:opts
								error:nil];
	}
	return img_set;
}

