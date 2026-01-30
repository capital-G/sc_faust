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

add_library(sc_faust MODULE
        src/sc_faust.cpp
        src/library.cpp
)

# Set the faust library path for runtime compilation
target_compile_definitions(sc_faust PRIVATE
    FAUST_LIBRARY_PATH="${SC_INSTALL_DIR}/${PROJECT_NAME}/externals/faustlibs"
)

# Copy faust libraries to build directory at configure time
set(FAUSTLIBS_BUILD_DIR "${CMAKE_BINARY_DIR}/externals/faustlibs")
file(COPY ${CMAKE_SOURCE_DIR}/external/faust/libraries/
    DESTINATION ${FAUSTLIBS_BUILD_DIR}
    FILES_MATCHING PATTERN "*.lib"
)

target_include_directories(sc_faust PUBLIC ${SC_PATH}/include/plugin_interface)
target_include_directories(sc_faust PUBLIC ${SC_PATH}/include/common)
target_include_directories(sc_faust PUBLIC ${SC_PATH}/common)

# Faust part
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

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

find_package(Threads REQUIRED)

if(WIN32)
    set(FAUST_STATIC_LIB "${FAUST_LIB}/libfaustwithllvm.lib")
    target_link_libraries(sc_faust
        ${FAUST_STATIC_LIB}
        Threads::Threads
        ws2_32
        ole32
        uuid
    )
else()
    set(FAUST_STATIC_LIB "${FAUST_LIB}/libfaustwithllvm.a")
    target_link_libraries(sc_faust
        ${FAUST_STATIC_LIB}
        Threads::Threads
        dl
        z
        m
        ncurses
    )
endif()

if(APPLE)
    target_link_libraries(sc_faust
        "-framework CoreServices"
        "-framework CoreFoundation"
    )
endif()

# install
set(INSTALL_DIR "${SC_INSTALL_DIR}/${PROJECT_NAME}")

install(TARGETS sc_faust LIBRARY DESTINATION ${INSTALL_DIR})

file(GLOB_RECURSE SC_FILES "${CMAKE_SOURCE_DIR}/src/sc/Classes/*.sc")
file(GLOB_RECURSE SCHELP_FILES "${CMAKE_SOURCE_DIR}/src/sc/HelpSource/*/**")
install(FILES ${SC_FILES} DESTINATION ${INSTALL_DIR}/Classes)
install(DIRECTORY src/sc/HelpSource DESTINATION ${INSTALL_DIR})
install(DIRECTORY external/faust/libraries/
    DESTINATION ${INSTALL_DIR}/externals/faustlibs
    FILES_MATCHING PATTERN "*.lib"
)
install(FILES "README.md" "LICENSE" DESTINATION "${INSTALL_DIR}")

