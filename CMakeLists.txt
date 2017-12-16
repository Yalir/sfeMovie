
cmake_minimum_required (VERSION 2.8.12)


# set a default build type if none was provided
# this has to be done before the project() instruction!
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

project (sfeMovie)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)

# Import required CMake files
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")
include(${CMAKE_SOURCE_DIR}/Shell.cmake)
include(${CMAKE_SOURCE_DIR}/FFmpeg/FFmpeg.cmake)

# Detect the OS
if(${CMAKE_HOST_WIN32})
    set(WINDOWS 1)
elseif(${CMAKE_HOST_APPLE})
    set(MACOSX 1)
elseif(${CMAKE_HOST_UNIX})
    set(LINUX 1)
else()
    message(FATAL_ERROR "Unsupported operating system")
    return()
endif()

if (WINDOWS AND NOT MSVC)
    message(FATAL_ERROR "Only Visual Studio (2013 and later) is supported on Windows")
endif()

# Set some env vars
set (SFEMOVIE_LIB sfeMovie)
set (VERSION_MAJOR 2)
set (VERSION_MINOR 0)

set (SOURCES_DIR "src")
set (PUBLIC_HEADERS "include")
set (PRIVATE_HEADERS "src")

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#################################################################################################################
# ============================================== FFMPEG SETUP ================================================= #
#################################################################################################################

set (SFEMOVIE_BUILD_FFMPEG TRUE CACHE BOOL "TRUE to build sfeMovie with the provided FFmpeg sources, FALSE to build with the system libraries")
add_subdirectory(FFmpeg)

#################################################################################################################
# ============================================== LIBASS SETUP ================================================= #
#################################################################################################################

set (SFEMOVIE_ENABLE_ASS_SUBTITLES FALSE CACHE BOOL "TRUE to build sfeMovie with libass, which enable support of non-bitmap subtitles (ie. srt")
if (SFEMOVIE_ENABLE_ASS_SUBTITLES)
    find_package (ASS REQUIRED)
    add_definitions(-DSFEMOVIE_ENABLE_ASS_SUBTITLES)
endif()

#################################################################################################################
# ================================================ SFML SETUP ================================================= #
#################################################################################################################

# Static sfeMovie lib has no interest on Linux or OS X
if (WINDOWS)
    set (SFEMOVIE_BUILD_STATIC FALSE CACHE BOOL "TRUE to build static version of sfeMovie")
endif()

if (SFEMOVIE_BUILD_STATIC)
    set (SFML_STATIC_LIBRARIES TRUE)
endif()

find_package (SFML 2.3 COMPONENTS graphics window system audio REQUIRED)
message(STATUS "SFML libraries: ${SFML_LIBRARIES}")
message(STATUS "Additional dependencies: ${SFML_DEPENDENCIES}")

#################################################################################################################
# ============================================== sfeMovie SETUP =============================================== #
#################################################################################################################

file(GLOB SOURCE_FILES ${SOURCES_DIR}/[a-z,A-Z]*.[c,h]* ${SOURCES_DIR}/DebugTools/[a-z,A-Z]*.[c,h]*)
file(GLOB HEADER_FILES RELATIVE ${PROJECT_SOURCE_DIR} "include/sfeMovie/[a-z,A-Z]*.[h]*")

add_definitions(-D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DSFE_EXPORTS)
if (APPLE) # ========================================== macOS ========================================== #
    # add an option to let the user specify a custom directory for framework installation
    set(CMAKE_INSTALL_FRAMEWORK_PREFIX "/Library/Frameworks" CACHE STRING "Frameworks installation directory")
endif()

# Set the search directories for compiling and link editing
set(SFEMOVIE_INCLUDE_DIRS "${SFML_INCLUDE_DIR};${PUBLIC_HEADERS};${PRIVATE_HEADERS};${FFMPEG_INCLUDE_DIRS}")
if (SFEMOVIE_ENABLE_ASS_SUBTITLES)
    set(SFEMOVIE_INCLUDE_DIRS "${SFEMOVIE_INCLUDE_DIRS};${LIBASS_INCLUDE_DIR}")
endif()

include_directories (${SFEMOVIE_INCLUDE_DIRS})
message (STATUS "Headers directories: ${SFEMOVIE_INCLUDE_DIRS}")

source_group("Public headers" FILES ${HEADER_FILES})
if (SFEMOVIE_BUILD_STATIC)
    add_library(${SFEMOVIE_LIB} STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
    add_library(${SFEMOVIE_LIB} SHARED ${SOURCE_FILES} ${HEADER_FILES})
endif()

if (NeedsFFmpegCompilation)
    add_dependencies(${SFEMOVIE_LIB} FFmpeg)
endif()

# Define the libraries sfeMovie should link against
if (SFEMOVIE_BUILD_STATIC)
    set_target_properties(${SFEMOVIE_LIB} PROPERTIES
        COMPILE_DEFINITIONS SFEMOVIE_STATIC
        COMPILE_DEFINITIONS SFML_STATIC)
    message(WARNING "You chose to build a static library of sfeMovie, this assumes that you will also statically link your software against SFML."
            "sfeMovie will be built with SFML_STATIC defined.")
else()
    set(SFEMOVIE_LIBRARIES ${SFML_LIBRARIES} ${FFMPEG_LIBRARIES} ${OTHER_LIBRARIES} ${SFML_DEPENDENCIES})
    if (SFEMOVIE_ENABLE_ASS_SUBTITLES)
        set(SFEMOVIE_LIBRARIES ${SFEMOVIE_LIBRARIES} ${LIBASS_LIBRARIES})
    endif()
    message(STATUS "Link sfeMovie against: ${LIBASS_LIBRARIES} ${SFML_LIBRARIES} ${FFMPEG_LIBRARIES} ${OTHER_LIBRARIES} ${SFML_DEPENDENCIES}")
    target_link_libraries(${SFEMOVIE_LIB} PRIVATE ${SFEMOVIE_LIBRARIES})
endif()

if (SFEMOVIE_BUILD_FFMPEG)
    if (LINUX)
        add_custom_command(TARGET ${SFEMOVIE_LIB} PRE_LINK
            COMMAND cp -R
                "${CMAKE_BINARY_DIR}/FFmpeg-binaries/lib/*"
                "$<TARGET_FILE_DIR:${SFEMOVIE_LIB}>")
    elseif(MACOSX)
        add_custom_command(TARGET ${SFEMOVIE_LIB} PRE_LINK
            COMMAND mkdir -p "$<TARGET_FILE_DIR:${SFEMOVIE_LIB}>/Libraries")
        add_custom_command(TARGET ${SFEMOVIE_LIB} PRE_LINK
            COMMAND cp -R
                "${CMAKE_BINARY_DIR}/FFmpeg-binaries/lib/*"
                "$<TARGET_FILE_DIR:${SFEMOVIE_LIB}>/Libraries")
    elseif(WINDOWS)
        add_custom_command(TARGET ${SFEMOVIE_LIB} PRE_LINK
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_BINARY_DIR}/FFmpeg-binaries/lib"
                "$<TARGET_FILE_DIR:${SFEMOVIE_LIB}>")
        add_custom_command(TARGET ${SFEMOVIE_LIB} PRE_LINK
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_BINARY_DIR}/FFmpeg-binaries/bin"
                "$<TARGET_FILE_DIR:${SFEMOVIE_LIB}>")
    endif()
endif()

include (${CMAKE_SOURCE_DIR}/cmake/Properties.cmake)
include (${CMAKE_SOURCE_DIR}/cmake/Install.cmake)

# Unit tests
add_subdirectory(tests)

# Examples building
add_subdirectory(examples)

# add an option for building the documentation
set(SFEMOVIE_BUILD_DOC FALSE CACHE BOOL "Set to true to build the documentation, requires Doxygen")
if(SFEMOVIE_BUILD_DOC)
    add_subdirectory(doxygen)
endif()

# Define packaging
include(${CMAKE_SOURCE_DIR}/cmake/Packaging.cmake)
