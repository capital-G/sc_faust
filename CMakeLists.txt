cmake_minimum_required(VERSION 3.15)
project(sc_faust)

set(CMAKE_CXX_STANDARD 17)

# SC Part
if(APPLE OR WIN32)
    set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
else()
    set(CMAKE_SHARED_MODULE_PREFIX "")
endif()

# default installation path - can be overridden with -DSC_INSTALL_DIR=...
if (WIN32)
    set(SC_INSTALL_DIR_DEFAULT "$ENV{LOCALAPPDATA}/SuperCollider/Extensions")
elseif(APPLE)
    set(SC_INSTALL_DIR_DEFAULT "~/Library/Application Support/SuperCollider/Extensions")
else()
    set(SC_INSTALL_DIR_DEFAULT "~/.local/share/SuperCollider/Extensions")
endif()
set(SC_INSTALL_DIR "${SC_INSTALL_DIR_DEFAULT}" CACHE PATH "SuperCollider Extensions installation directory")
message(STATUS "SC_INSTALL_DIR: ${SC_INSTALL_DIR}")

# faust libraries
file (GLOB DSPLIBS external/faustlibraries/*.lib)

add_library(sc_faust MODULE
        src/sc_faust.cpp
        src/library.cpp
)

# Set the faust library path for runtime compilation
target_compile_definitions(sc_faust PRIVATE
    FAUST_LIBRARY_PATH="${SC_INSTALL_DIR}/${PROJECT_NAME}/externals/faustlibs"
)

# Copy faust libraries to build directory during build
set(FAUSTLIBS_BUILD_DIR "${CMAKE_BINARY_DIR}/externals/faustlibs")
file(MAKE_DIRECTORY ${FAUSTLIBS_BUILD_DIR})
add_custom_command(TARGET sc_faust POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DSPLIBS} ${FAUSTLIBS_BUILD_DIR}
    COMMENT "Copying Faust libraries to build directory"
)

target_include_directories(sc_faust PUBLIC ${SC_PATH}/include/plugin_interface)
target_include_directories(sc_faust PUBLIC ${SC_PATH}/include/common)
target_include_directories(sc_faust PUBLIC ${SC_PATH}/common)

# Faust part
set (CMAKE_CXX_FLAGS_RELEASE "-O3")

set(FAUST_SRC_DIR "${CMAKE_SOURCE_DIR}/external/faust" CACHE PATH "Path to Faust source repository")

set(FAUST_LIB_DIR "" CACHE PATH "Path to pre-built Faust libraries (containing lib/)")
if(NOT FAUST_LIB_DIR)
    message(FATAL_ERROR "FAUST_LIB_DIR must be set to the path containing pre-built Faust libraries")
endif()

set(FAUST_INCLUDE "${FAUST_SRC_DIR}/architecture")
set(FAUST_LIB "${FAUST_LIB_DIR}/lib")

message(STATUS "Using Faust headers from ${FAUST_INCLUDE}")
message(STATUS "Using Faust libraries from ${FAUST_LIB}")

target_include_directories(sc_faust PRIVATE ${FAUST_INCLUDE})

set(FAUST_STATIC_LIB "${FAUST_LIB}/libfaustwithllvm.a")

find_package(Threads REQUIRED)
target_link_libraries(sc_faust
        ${FAUST_STATIC_LIB}
        Threads::Threads
        dl
        z
        m
        ncurses
)

if (APPLE)
    target_link_libraries(sc_faust
        "-framework CoreServices"
        "-framework CoreFoundation"
    )
endif()

if(NOT UNIX)
    message(FATAL_ERROR "Only macOS and linux is currently supported")
endif()

# install
install(TARGETS sc_faust LIBRARY DESTINATION ${PROJECT_NAME})

file(GLOB_RECURSE SC_FILES "${CMAKE_SOURCE_DIR}/src/sc/Classes/*.sc")
install(FILES ${SC_FILES} DESTINATION ${PROJECT_NAME}/Classes)
install(FILES ${DSPLIBS} DESTINATION ${PROJECT_NAME}/externals/faustlibs)

message( STATUS "DSP LIBS IS ${DSPLIBS}")
