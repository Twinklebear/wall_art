#import <Foundation/Foundation.h>
#import <Appkit/AppKit.h>

bool osx_set_background(const char *path) {
	NSLog(@"Now in objective c land\n");
	NSString *nspath = [[NSString string] initWithUTF8String:path];
	NSLog(@"mand nspath\n");
	NSLog(@"%@", nspath);
	NSURL *url = [NSURL fileURLWithPath:nspath isDirectory:NO];
	NSLog(@"Made nsurl");
	NSLog(@"%@", url);
	// The default options are what we want actually
	NSDictionary *opts = @{};
	// TODO: Loop through all screens and set wallpaper on each
	NSLog(@"Calling set desktop url\n");
	return [[NSWorkspace sharedWorkspace] setDesktopImageURL:url
			forScreen:[NSScreen mainScreen]
			options:opts
			error:nil];
}

