
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
set (VERSION_MAJOR 2)
set (VERSION_MINOR 0)

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

#################################################################################################################
# ============================================== sfeMovie SETUP =============================================== #
#################################################################################################################

file(GLOB SOURCE_FILES src/[a-z,A-Z]*.[c,h]* src/DebugTools/[a-z,A-Z]*.[c,h]*)
file(GLOB HEADER_FILES RELATIVE ${PROJECT_SOURCE_DIR} "include/sfeMovie/[a-z,A-Z]*.[h]*")

if (APPLE) # ========================================== macOS ========================================== #
    # add an option to let the user specify a custom directory for framework installation
    set(CMAKE_INSTALL_FRAMEWORK_PREFIX "/Library/Frameworks" CACHE STRING "Frameworks installation directory")
endif()

source_group("Public headers" FILES ${HEADER_FILES})
if (SFEMOVIE_BUILD_STATIC)
    add_library(sfeMovie STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
    add_library(sfeMovie SHARED ${SOURCE_FILES} ${HEADER_FILES})
endif()

add_library(sfeMovie_PrivateHeaders INTERFACE)
target_include_directories(sfeMovie_PrivateHeaders INTERFACE src ${FFMPEG_INCLUDE_DIRS})

target_link_libraries(sfeMovie PRIVATE sfeMovie_PrivateHeaders)
target_include_directories(sfeMovie
    PUBLIC include ${SFML_INCLUDE_DIR}
    PRIVATE src ${FFMPEG_INCLUDE_DIRS})
if (SFEMOVIE_ENABLE_ASS_SUBTITLES)
    target_include_directories(sfeMovie PRIVATE ${LIBASS_INCLUDE_DIR})
endif()

target_compile_definitions(sfeMovie PRIVATE __STDC_CONSTANT_MACROS __STDC_LIMIT_MACROS SFE_EXPORTS)



if (NeedsFFmpegCompilation)
    add_dependencies(sfeMovie FFmpeg)
endif()

# Define the libraries sfeMovie should link against
if (SFEMOVIE_BUILD_STATIC)
    set_target_properties(sfeMovie PROPERTIES
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
    target_link_libraries(sfeMovie PRIVATE ${SFEMOVIE_LIBRARIES})
endif()

if (SFEMOVIE_BUILD_FFMPEG)
    if (LINUX)
        add_custom_command(TARGET sfeMovie PRE_LINK
            COMMAND cp -R
                "${CMAKE_BINARY_DIR}/FFmpeg-binaries/lib/*"
                "$<TARGET_FILE_DIR:sfeMovie>")
    elseif(MACOSX)
        add_custom_command(TARGET sfeMovie PRE_LINK
            COMMAND mkdir -p "$<TARGET_FILE_DIR:sfeMovie>/Libraries")
        add_custom_command(TARGET sfeMovie PRE_LINK
            COMMAND cp -R
                "${CMAKE_BINARY_DIR}/FFmpeg-binaries/lib/*"
                "$<TARGET_FILE_DIR:sfeMovie>/Libraries")
    elseif(WINDOWS)
        add_custom_command(TARGET sfeMovie PRE_LINK
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_BINARY_DIR}/FFmpeg-binaries/lib"
                "$<TARGET_FILE_DIR:sfeMovie>")
        add_custom_command(TARGET sfeMovie PRE_LINK
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_BINARY_DIR}/FFmpeg-binaries/bin"
                "$<TARGET_FILE_DIR:sfeMovie>")
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
