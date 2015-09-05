QT5_WRAP_CPP(wall_art_app.cpp image_downloader.cpp)
add_executable(wall_art main.cpp wall_art_app.cpp image_downloader.cpp)
target_link_libraries(wall_art Shcore.lib Qt5::Widgets Qt5::Network)
install(TARGETS wall_art DESTINATION "${WALL_ART_INSTALL_DIR}")

