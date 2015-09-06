#define _USE_MATH_DEFINES
#include <iostream>

#include "background_builder.h"
#include "wall_art_app.h"

int main(int argc, char **argv){
	if (argc < 2){
		std::cout << "Usage ./image_blur <image_id>\n";
		return 1;
	}
	qRegisterMetaType<ImageResult>();
	qRegisterMetaType<QSharedPointer<QImage>>();
	WallArtApp app{argc, argv};
	app.build_background(std::stoi(argv[1]));
	return app.exec();
}

