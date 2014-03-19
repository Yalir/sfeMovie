
cmake_minimum_required (VERSION 2.8)


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

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_definitions("-O2")
endif()

#################################################################################################################
# ============================================== FFMPEG SETUP ================================================= #
#################################################################################################################

set (LINK_AGAINST_INTERNAL_FFMPEG TRUE CACHE BOOL "TRUE to build sfeMovie with the provided FFmpeg sources, FALSE to build with the system libraries")
add_subdirectory(FFmpeg)

#################################################################################################################
# ================================================ SFML SETUP ================================================= #
#################################################################################################################

find_package (SFML 2 COMPONENTS graphics window system audio REQUIRED)
set(SFML_LIBRARIES ${SFML_GRAPHICS_LIBRARY} ${SFML_WINDOW_LIBRARY} ${SFML_AUDIO_LIBRARY} ${SFML_SYSTEM_LIBRARY})

#################################################################################################################
# =============================================== Boost SETUP ================================================= #
#################################################################################################################

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.46 COMPONENTS thread system REQUIRED)

#################################################################################################################
# ============================================== sfeMovie SETUP =============================================== #
#################################################################################################################

file(GLOB SOURCE_FILES ${SOURCES_DIR}/[a-z,A-Z]*.[c,h]*)

if (LINUX) # ========================================== LINUX ========================================== #
    add_definitions(-D__STDC_CONSTANT_MACROS)
	set (SOURCE_FILES ${SOURCE_FILES} "${SOURCES_DIR}/Unix/ConditionImpl.cpp")

elseif (MACOSX) # ========================================== MACOSX ========================================== #
	set (SOURCE_FILES ${SOURCE_FILES} "${SOURCES_DIR}/Unix/ConditionImpl.cpp")
    
    # add an option to let the user specify a custom directory for framework installation
    set(CMAKE_INSTALL_FRAMEWORK_PREFIX "/Library/Frameworks" CACHE STRING "Frameworks installation directory")
	
	# find only "root" headers
    file(GLOB SFE_HEADERS RELATIVE ${PROJECT_SOURCE_DIR} "include/sfeMovie/*")
    
    # Make sure sfeMovie headers get imported in the framework
    set (SOURCE_FILES ${SOURCE_FILES} ${SFE_HEADERS})
elseif (WINDOWS) # ========================================== WINDOWS ==========================================
	set (SOURCE_FILES ${SOURCE_FILES} "${SOURCES_DIR}/Win32/ConditionImpl.cpp")

	if (MSVC)
		set (PRIVATE_HEADERS ${PRIVATE_HEADERS} "${CMAKE_SOURCE_DIR}/deps/headers/msvc")
	else()
		set (LINKER_FLAGS ${LINKER_FLAGS} "-static-libgcc")
		find_library(LIBZ z PATHS "${CMAKE_SOURCE_DIR}/deps/windows-binaries/gcc")

		if (NOT LIBZ)
			message(FATAL_ERROR "Could not find required library libz")
		endif()
		set (OTHER_LIBRARIES ${OTHER_LIBRARIES} "${LIBZ}")
	endif()
	
	add_definitions(-D__STDC_CONSTANT_MACROS -DSFE_EXPORTS)
	set (OTHER_LIBRARIES ${OTHER_LIBRARIES} "ws2_32")
endif()

source_group(Sources FILES ${SOURCE_FILES})

# Set the search directories for compiling and link editing
include_directories (${SFML_INCLUDE_DIR} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS} ${FFMPEG_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})
message (STATUS "Headers directories: ${SFML_INCLUDE_DIR} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS} ${FFMPEG_INCLUDE_DIR} ${Boost_INCLUDE_DIRS}")

# Define the source files sfe should compile
add_library (${SFEMOVIE_LIB} SHARED ${SOURCE_FILES})
add_dependencies(${SFEMOVIE_LIB} FFmpeg)

# Define the libraries sfe should link against
target_link_libraries (${SFEMOVIE_LIB} ${SFML_LIBRARIES} ${FFMPEG_LIBRARIES} ${OTHER_LIBRARIES} ${Boost_LIBRARIES})

if (MACOSX)
	target_link_libraries(${SFEMOVIE_LIB} LINK_INTERFACE_LIBRARIES)
endif()

if (${LINK_AGAINST_INTERNAL_FFMPEG})
	if (LINUX)
		add_custom_command(TARGET ${SFEMOVIE_LIB} POST_BUILD
			COMMAND cp -R
				"${CMAKE_BINARY_DIR}/FFmpeg-binaries/lib/*"
				$<TARGET_FILE_DIR:${SFEMOVIE_LIB}>)
	elseif(MACOSX)
		add_custom_command(TARGET ${SFEMOVIE_LIB} POST_BUILD
			COMMAND cp -R
				"${CMAKE_BINARY_DIR}/FFmpeg-binaries/lib/*"
				$<TARGET_FILE_DIR:${SFEMOVIE_LIB}>/Libraries)
	elseif(WINDOWS)
		add_custom_command(TARGET ${SFEMOVIE_LIB} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_directory
				"${CMAKE_BINARY_DIR}/FFmpeg-binaries/lib"
				$<TARGET_FILE_DIR:${SFEMOVIE_LIB}>)
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

# Demuxer command line tool
set(BUILD_DEMUXER_CLI FALSE CACHE BOOL "TRUE to build the Demuxer command line program (to check file contents and play audio)")
if (BUILD_DEMUXER_CLI)
	add_subdirectory(DemuxerCLI)
endif()

# add an option for building the documentation
set(BUILD_DOC FALSE CACHE BOOL "Set to true to build the documentation")
if(BUILD_DOC)
    add_subdirectory(doxygen)
endif()
