
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

# Set some env vars
set (SFEMOVIE_LIB sfeMovie)
set (VERSION_MAJOR 2)
set (VERSION_MINOR 0)

set (SOURCES_DIR "src")
set (PUBLIC_HEADERS "include")
set (PRIVATE_HEADERS "src")
set (OTHER_LIBRARIES "")
set (LINKER_FLAGS "")

#################################################################################################################
# ============================================== FFMPEG SETUP ================================================= #
#################################################################################################################

set (LINK_AGAINST_INTERNAL_FFMPEG TRUE CACHE BOOL "TRUE to build sfeMovie with the provided FFmpeg sources, FALSE to build with the system libraries")
add_subdirectory(FFmpeg)

#################################################################################################################
# ================================================ SFML SETUP ================================================= #
#################################################################################################################

set (SFEMOVIE_BUILD_STATIC FALSE CACHE BOOL "TRUE to build static version of sfeMovie")

if (NOT SFEMOVIE_BUILD_STATIC)
    # set (SFML_STATIC_LIBRARIES FALSE CACHE BOOL "TRUE to link sfeMovie against static SFML libraries instead of dynamic ones")
    find_package (SFML 2 COMPONENTS graphics window system audio REQUIRED)
    message(STATUS "SFML libraries: ${SFML_LIBRARIES}")
    message(STATUS "Additional dependencies: ${SFML_DEPENDENCIES}")
endif()

#################################################################################################################
# ============================================== sfeMovie SETUP =============================================== #
#################################################################################################################

file(GLOB SOURCE_FILES ${SOURCES_DIR}/[a-z,A-Z]*.[c,h]* ${SOURCES_DIR}/DebugTools/[a-z,A-Z]*.[c,h]*)

add_definitions(-D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DSFE_EXPORTS)
if (MACOSX) # ========================================== MACOSX ========================================== #
    # add an option to let the user specify a custom directory for framework installation
    set(CMAKE_INSTALL_FRAMEWORK_PREFIX "/Library/Frameworks" CACHE STRING "Frameworks installation directory")
    
    # find only "root" headers
    file(GLOB SFE_HEADERS RELATIVE ${PROJECT_SOURCE_DIR} "include/sfeMovie/[a-z,A-Z]*.[h]*")
    
    # Make sure sfeMovie headers get imported in the framework
    set (SOURCE_FILES ${SOURCE_FILES} ${SFE_HEADERS})
elseif (WINDOWS) # ========================================== WINDOWS ==========================================
    if (MSVC)
        set (PRIVATE_HEADERS ${PRIVATE_HEADERS} "${CMAKE_SOURCE_DIR}/deps/headers/msvc")
    else()
        find_library(LIBZ z PATHS "${CMAKE_SOURCE_DIR}/deps/Windows-i386")

        if (NOT LIBZ)
            message(FATAL_ERROR "Could not find required library libz")
        else()
            set (OTHER_LIBRARIES ${OTHER_LIBRARIES} "${LIBZ}")
        endif()
    endif()
    
    set (OTHER_LIBRARIES ${OTHER_LIBRARIES} "ws2_32")
endif()

source_group(Sources FILES ${SOURCE_FILES})

# Set the search directories for compiling and link editing
set(SFEMOVIE_INCLUDE_DIRS "${SFML_INCLUDE_DIR};${PUBLIC_HEADERS};${PRIVATE_HEADERS};${FFMPEG_INCLUDE_DIRS}")
include_directories (${SFEMOVIE_INCLUDE_DIRS})
message (STATUS "Headers directories: ${SFML_INCLUDE_DIR} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS} ${FFMPEG_INCLUDE_DIRS}")

# Define the source files sfe should compile
if (SFEMOVIE_BUILD_STATIC)
    add_library(${SFEMOVIE_LIB} STATIC ${SOURCE_FILES})
else()
    add_library (${SFEMOVIE_LIB} SHARED ${SOURCE_FILES})
endif()
add_dependencies(${SFEMOVIE_LIB} FFmpeg)

# Define the libraries sfe should link against
if (NOT SFEMOVIE_BUILD_STATIC)
    message(STATUS "Link sfeMovie against: ${SFML_LIBRARIES} ${FFMPEG_LIBRARIES} ${OTHER_LIBRARIES} ${SFML_DEPENDENCIES}")
	target_link_libraries(${SFEMOVIE_LIB} PRIVATE ${SFML_LIBRARIES} ${FFMPEG_LIBRARIES} ${OTHER_LIBRARIES} ${SFML_DEPENDENCIES})
else()
	set_target_properties(${SFEMOVIE_LIB} PROPERTIES
		COMPILE_DEFINITIONS SFEMOVIE_STATIC
		COMPILE_DEFINITIONS SFML_STATIC)
endif()

if (${LINK_AGAINST_INTERNAL_FFMPEG})
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
    endif()
endif()

include (${CMAKE_SOURCE_DIR}/cmake/Properties.cmake)
include (${CMAKE_SOURCE_DIR}/cmake/Install.cmake)

# Unit tests
set (BUILD_UNIT_TESTS FALSE CACHE BOOL "TRUE to build the unit test system")
if (BUILD_UNIT_TESTS)
    add_subdirectory(tests)
endif()

# Sample building
set (BUILD_SFEMOVIE_SAMPLE FALSE CACHE BOOL "TRUE to build the sfeMovie sample")
if (BUILD_SFEMOVIE_SAMPLE)
    add_subdirectory(sample)
endif ()

# add an option for building the documentation
set(BUILD_DOC FALSE CACHE BOOL "Set to true to build the documentation")
if(BUILD_DOC)
    add_subdirectory(doxygen)
endif()

set(BUILD_PACKAGE FALSE CACHE BOOL "Set to true to generate a zip file containing all the files required for development with sfeMovie")
if (BUILD_PACKAGE)
    include(${CMAKE_SOURCE_DIR}/cmake/Packaging.cmake)
endif()
