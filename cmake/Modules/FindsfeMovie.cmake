# This script locates the sfeMovie library.
# https://github.com/Yalir/sfeMovie
#
# USAGE
#
# By default, the dynamic version of sfeMovie will be found. To find the static
# one instead, you must set the SFEMOVIE_STATIC_LIBRARIES variable to TRUE before
# calling find_package( sfeMovie ). In that case SFEMOVIE_STATIC will also be defined
# by this script. Example:
#
# set( SFEMOVIE_STATIC_LIBRARIES TRUE )
# find_package( sfeMovie )
#
# If sfeMovie is not installed in a standard path, you can set
# SFEMOVIE_ROOT CMake (or environment) variables to tell CMake where to look for
# sfeMovie.
#
#
# OUTPUT
#
# This script defines the following variables:
#   - SFEMOVIE_LIBRARY_DEBUG:   the path to the debug library
#   - SFEMOVIE_LIBRARY_RELEASE: the path to the release library
#   - SFEMOVIE_LIBRARY:         the path to the library to link to for current configuration
#   - SFEMOVIE_FOUND:           true if the sfeMovie library is found
#   - SFEMOVIE_INCLUDE_DIR:     the path where sfeMovie headers are located (the directory containing the sfeMovie/Movie.hpp file)
#
#
# EXAMPLE
#
# find_package( sfeMovie REQUIRED )
# include_directories( ${SFEMOVIE_INCLUDE_DIR} )
# add_executable( myapp ... )
# target_link_libraries( myapp ${SFEMOVIE_LIBRARY} ... )

include(FindPackageHandleStandardArgs)

if( SFEMOVIE_STATIC_LIBRARIES )
	set( SFEMOVIE_SUFFIX "-s" )
	add_definitions( -DSFEMOVIE_STATIC )
else()
	set( SFEMOVIE_SUFFIX "" )
endif()

set (LIBRARY_SEARCH_PATHS
	/usr
	/usr/local
	${SFEMOVIE_ROOT}
	$ENV{SFEMOVIE_ROOT})

find_path(
	SFEMOVIE_INCLUDE_DIR sfeMovie/Movie.hpp
	PATH_SUFFIXES include
	PATHS ${LIBRARY_SEARCH_PATHS}
)

find_library(
	SFEMOVIE_LIBRARY_RELEASE
	sfeMovie${SFEMOVIE_SUFFIX}
	PATH_SUFFIXES
		lib
		lib64
	PATHS ${LIBRARY_SEARCH_PATHS}
)

find_library(
	SFEMOVIE_LIBRARY_DEBUG
	sfeMovie${SFEMOVIE_SUFFIX}-d
	PATH_SUFFIXES
		lib
		lib64
	PATHS ${LIBRARY_SEARCH_PATHS}
)

if( SFEMOVIE_LIBRARY_RELEASE AND SFEMOVIE_LIBRARY_DEBUG )
	set( SFEMOVIE_LIBRARY debug ${SFEMOVIE_LIBRARY_DEBUG} optimized ${SFEMOVIE_LIBRARY_RELEASE} )
endif()

if( SFEMOVIE_LIBRARY_RELEASE AND NOT SFEMOVIE_LIBRARY_DEBUG )
	set( SFEMOVIE_LIBRARY_DEBUG ${SFEMOVIE_LIBRARY_RELEASE} )
	set( SFEMOVIE_LIBRARY ${SFEMOVIE_LIBRARY_RELEASE} )
endif()

if( SFEMOVIE_LIBRARY_DEBUG AND NOT SFEMOVIE_LIBRARY_RELEASE )
	set( SFEMOVIE_LIBRARY_RELEASE ${SFEMOVIE_LIBRARY_DEBUG} )
	set( SFEMOVIE_LIBRARY ${SFEMOVIE_LIBRARY_DEBUG} )
endif()

# handle the QUIETLY and REQUIRED arguments and set SFEMOVIE_FOUND to TRUE if
# all listed variables are TRUE
find_package_handle_standard_args(sfeMovie 
	FAIL_MESSAGE "Could NOT find sfeMovie. Make sure it is installed or define SFEMOVIE_ROOT if it is located in a custom path"
    REQUIRED_VARS SFEMOVIE_LIBRARY SFEMOVIE_INCLUDE_DIR)
