QT5_WRAP_CPP(wall_art_app.cpp image_downloader.cpp background_builder.cpp)
add_executable(wall_art main.cpp wall_art_app.cpp image_downloader.cpp background_builder.cpp
	image_data.cpp)
target_link_libraries(wall_art Qt5::Widgets Qt5::Network)
install(TARGETS wall_art DESTINATION "${WALL_ART_INSTALL_DIR}")

