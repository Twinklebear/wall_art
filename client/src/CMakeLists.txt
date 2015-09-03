add_executable(wall_art WIN32 main.cpp)
target_link_libraries(wall_art Shcore.lib Qt5::Widgets)
install(TARGETS wall_art DESTINATION "${WALL_ART_INSTALL_DIR}")

