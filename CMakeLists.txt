cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(spdrwc VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")

option(ADD_DEBUG_EXTRA_OPTS "Add extra debug flags and technique" OFF)

# Find the Qt6 libraries
find_package(Qt6 REQUIRED COMPONENTS Core)
if (UNIX)
    get_target_property(QT_CORE_TYPE Qt6::Core TYPE)
    if(QT_IS_STATIC OR QT_CORE_TYPE STREQUAL "STATIC_LIBRARY")
        # Required when using with Qt 6 static libraries
        message(STATUS "Qt6 is static. Finding libudev via pkg-config for QtSerialPort...")
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(Libudev REQUIRED IMPORTED_TARGET libudev)
    endif()
endif()
find_package(Qt6 REQUIRED COMPONENTS SerialPort)
#include(Qt6TranslationToolsMacros)

# Qt6 modules can override the C++ standard, here we have set the minimum required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-DVERSION=\"${PROJECT_VERSION}\")

if(ADD_DEBUG_EXTRA_OPTS)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_definitions(-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC)
        set(DEBUG_EXTRA_FLAGS "-fstack-protector -fstack-clash-protection -fno-omit-frame-pointer -Wshift-overflow=2")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(DEBUG_EXTRA_FLAGS "-fstack-check -fstack-protector -fno-omit-frame-pointer -Wshift-overflow")
    endif()
    if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
        set (DEBUG_EXTRA_FLAGS "${DEBUG_EXTRA_FLAGS} -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fno-sanitize=alignment")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-check -fstack-protector")
        set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-check -fstack-protector")
    endif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEBUG_EXTRA_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEBUG_EXTRA_FLAGS}")
endif(ADD_DEBUG_EXTRA_OPTS)

if (WIN32)
    set(CMAKE_RC_COMPILER windres)
    # set rc syntax
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> <DEFINES> -O coff -o <OBJECT> -i <SOURCE>")
    set(CMAKE_RC_SOURCE_FILE_EXTENSIONS rc)
    set(CMAKE_RC_FLAGS "-I${CMAKE_BINARY_DIR}")
    # enable resource language for this project
    enable_language(RC)
endif(WIN32)

add_subdirectory(src)
