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

if(UNIX AND NOT APPLE)
    # Linux: use pre-built Faust libraries + source repo for headers
    set(FAUST_LIB_DIR "" CACHE PATH "Path to pre-built Faust libraries (containing lib/)")
    set(FAUST_SRC_DIR "" CACHE PATH "Path to Faust source repository (for headers in architecture/)")

    if(NOT FAUST_LIB_DIR)
        message(FATAL_ERROR "FAUST_LIB_DIR must be set to the path containing pre-built Faust libraries on Linux")
    endif()
    if(NOT FAUST_SRC_DIR)
        message(FATAL_ERROR "FAUST_SRC_DIR must be set to the path of the Faust source repository on Linux")
    endif()

    set(FAUST_INCLUDE "${FAUST_SRC_DIR}/architecture")
    set(FAUST_LIB "${FAUST_LIB_DIR}/lib")

    message(STATUS "Using Faust headers from ${FAUST_INCLUDE}")
    message(STATUS "Using Faust libraries from ${FAUST_LIB}")

    target_include_directories(sc_faust PRIVATE ${FAUST_INCLUDE})

    # Static linking with libfaustwithllvm.a (self-contained, no runtime dependency)
    set(FAUST_STATIC_LIB "${FAUST_LIB}/libfaustwithllvm.a")

    # System libraries required by LLVM/Faust
    find_package(Threads REQUIRED)
    target_link_libraries(sc_faust
        ${FAUST_STATIC_LIB}
        Threads::Threads
        dl
        z
        m
        ncurses
    )
else()
    # macOS: use system faust (e.g., from Homebrew)
    if (NOT DEFINED FAUST)
        set (FAUST faust)
    endif()

    message (STATUS "Using Faust as ${FAUST}")
    execute_process (COMMAND ${FAUST} --includedir OUTPUT_VARIABLE FAUST_INCLUDE)
    string ( STRIP "${FAUST_INCLUDE}" FAUST_INCLUDE )
    message (STATUS "Using Faust include from ${FAUST_INCLUDE}")
    execute_process (COMMAND ${FAUST} --libdir OUTPUT_VARIABLE FAUST_LIB)
    string ( STRIP "${FAUST_LIB}" FAUST_LIB )
    message (STATUS "Using Faust libraries from ${FAUST_LIB}")

    target_include_directories(sc_faust PRIVATE ${FAUST_INCLUDE})

    execute_process( COMMAND faust-config --system-libs OUTPUT_VARIABLE FC_OUTPUT RESULT_VARIABLE FC_RESULT)
    string(REPLACE " " ";" FC_OUTPUT ${FC_OUTPUT})
    string(STRIP "${FC_OUTPUT}" FC_OUTPUT)

    set (FAUST_LIBS ${FC_OUTPUT} ${FAUST_LIB}/libOSCFaust.dylib ${FAUST_LIB}/libfaust.dylib ${FAUST_LIB}/libfaustmachine.dylib)
    target_link_libraries (sc_faust ${FAUST_LIBS})
endif()

message (STATUS "Faust libs: ${FAUST_LIBS}")

# install
install(TARGETS sc_faust LIBRARY DESTINATION ${PROJECT_NAME})

file(GLOB_RECURSE SC_FILES "${CMAKE_SOURCE_DIR}/src/sc/Classes/*.sc")
install(FILES ${SC_FILES} DESTINATION ${PROJECT_NAME}/Classes)
install(FILES ${DSPLIBS} DESTINATION ${PROJECT_NAME}/externals/faustlibs)

message( STATUS "DSP LIBS IS ${DSPLIBS}")
