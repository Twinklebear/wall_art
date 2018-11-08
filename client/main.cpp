#include "background_builder.h"
#include "wall_art_app.h"

int main(int argc, char **argv){
	qRegisterMetaType<ImageResult>();
	qRegisterMetaType<QSharedPointer<QImage>>();
	WallArtApp app{argc, argv};
	if (argc == 2){
		app.build_background(std::stoi(argv[1]));
	}
	return app.exec();
}

