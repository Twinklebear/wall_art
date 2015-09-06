#define _USE_MATH_DEFINES
#include <iostream>
#include <chrono>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <vector>
#include <cmath>
#include <string>
#include <cstring>
#include <memory>
#include <wtypes.h>
#include <windows.h>
#include <ShellScalingAPI.h>
#include "background_builder.h"
#include "wall_art_app.h"

int main(int argc, char **argv){
	if (argc < 2){
		std::cout << "Usage ./image_blur <image_id>\n";
		return 1;
	}
	// Tell Windows we're DPI aware so we can get the true resolution of
	// the display
	SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);

	qRegisterMetaType<ImageResult>();
	qRegisterMetaType<QSharedPointer<QImage>>();
	WallArtApp app{argc, argv};
	app.build_background(std::stoi(argv[1]));
	return app.exec();
}

