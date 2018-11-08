cmake_minimum_required(VERSION 2.8.12)
project(WALL_ART)

# Bump up warning levels appropriately for each compiler
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -Wall -Wextra -pedantic -std=c++14 -fopenmp -ffast-math")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -ggdb -DDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -march=native -flto -fno-use-linker-plugin -DNDEBUG -fno-exceptions")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto -fno-exceptions")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /Zi")
	endif()
endif()

find_package(OpenMP)
if (OPENMP_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# Hack to find Qt on windows
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt5Widgets)
find_package(Qt5Network)

set(SOURCE_FILES main.cpp wall_art_app.cpp image_downloader.cpp background_builder.cpp image_data.cpp)
if (WIN32)
	set(SOURCE_FILES ${SOURCE_FILES} win32_background.cpp)
endif()
# TODO: For win32 later we want to set this as a win32 app
add_executable(wall_art ${SOURCE_FILES})

target_link_libraries(wall_art Qt5::Widgets Qt5::Network)
install(TARGETS wall_art DESTINATION bin)

