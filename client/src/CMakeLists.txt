QT5_WRAP_CPP(wall_art_app.cpp image_downloader.cpp background_builder.cpp)
set(SOURCE_FILES main.cpp wall_art_app.cpp image_downloader.cpp background_builder.cpp image_data.cpp)
if (WIN32)
	set(SOURCE_FILES ${SOURCE_FILES} win32_background.cpp)
endif()
add_executable(wall_art ${SOURCE_FILES})

target_link_libraries(wall_art Qt5::Widgets Qt5::Network)
install(TARGETS wall_art DESTINATION "${WALL_ART_INSTALL_DIR}")

