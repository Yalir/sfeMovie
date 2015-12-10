# - Try to find libass
# Once done this will define
#  LIBASS_FOUND - System has libass
#  LIBASS_INCLUDE_DIRS - The libass include directories
#  LIBASS_LIBRARIES - The libraries needed to use libass
#  LIBASS_DEFINITIONS - Compiler switches required for using libass

include(FindPackageHandleStandardArgs)

find_package(PkgConfig)
pkg_check_modules(PC_LIBASS QUIET libass)
set(LIBASS_DEFINITIONS ${PC_LIBASS_CFLAGS_OTHER})
set(LIBRARY_SEARCH_PATHS
  ${LIBASS_ROOT}/lib
  $ENV{LIBASS_ROOT}/lib
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local/lib
  /usr/local/lib64
  /usr/local/lib/${CMAKE_LIBRARY_ARCHITECTURE}
  /usr/lib
  /usr/lib64
  /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
  /sw/lib
  /opt/local/lib
  /opt/csw/lib
  /opt/lib
  /usr/freeware/lib64)

find_path(LIBASS_INCLUDE_DIR ass/ass.h
          HINTS ${PC_LIBASS_INCLUDEDIR} ${PC_LIBASS_INCLUDE_DIRS}
          PATHS
          ${LIBASS_ROOT}/include
          $ENV{LIBASS_ROOT}/include
          ~/Library/Frameworks
          /Library/Frameworks
          /usr/local/include
          /usr/include
          /sw/include # Fink
          /opt/local/include # DarwinPorts
          /opt/csw/include # Blastwave
          /opt/include
          /usr/freeware/include
          PATH_SUFFIXES libass)

find_library(LIBASS_LIBRARY NAMES ass
             PATHS ${LIBRARY_SEARCH_PATHS}
             HINTS ${PC_LIBASS_LIBDIR} ${PC_LIBASS_LIBRARY_DIRS})

set(LIBASS_LIBRARIES ${LIBASS_LIBRARY} )
set(LIBASS_INCLUDE_DIRS ${LIBASS_INCLUDE_DIR} )

# On Windows we assume that only static version of libass is used (this is the default build type)
# so we also link against its dependencies (libass does not provide any Find*.cmake script to simplify this aspect)
if (${CMAKE_HOST_WIN32})
  find_library(FREETYPE_LIBRARY NAMES freetype
               PATHS ${LIBRARY_SEARCH_PATHS} ${FREETYPE_ROOT} $ENV{FREETYPE_ROOT}
               HINTS ${PC_LIBASS_LIBDIR} ${PC_LIBASS_LIBRARY_DIRS})
  
  find_library(FRIBIDI_LIBRARY NAMES fribidi
               PATHS ${LIBRARY_SEARCH_PATHS} ${FRIBIDI_ROOT} $ENV{FRIBIDI_ROOT}
               HINTS ${PC_LIBASS_LIBDIR} ${PC_LIBASS_LIBRARY_DIRS})

  find_library(HARBUZZ_LIBRARY NAMES harbuzz
               PATHS ${LIBRARY_SEARCH_PATHS} ${HARFBUZZ_ROOT} $ENV{HARFBUZZ_ROOT}
               HINTS ${PC_LIBASS_LIBDIR} ${PC_LIBASS_LIBRARY_DIRS})

  if (NOT FREETYPE_LIBRARY)
    message(FATAL_ERROR "Could not find required FreeType library: define FREETYPE_LIBRARY to the location of your FreeType library")
  endif()

  if (NOT FRIBIDI_LIBRARY)
    message(FATAL_ERROR "Could not find required Fribidi library: define FRIBIDI_LIBRARY to the location of your Fribidi library")
  endif()

  if (NOT HARBUZZ_LIBRARY)
    message(FATAL_ERROR "Could not find required Harfbuzz library: define HARBUZZ_LIBRARY to the location of your Harfbuzz library")
  endif()

  set(LIBASS_LIBRARIES ${LIBASS_LIBRARIES} ${FREETYPE_LIBRARY} ${FRIBIDI_LIBRARY} ${HARBUZZ_LIBRARY})
endif()

# handle the QUIETLY and REQUIRED arguments and set CURL_FOUND to TRUE if
# all listed variables are TRUE
find_package_handle_standard_args(ASS "Could not find libass. Make sure it is installed or define LIBASS_ROOT if it is located in a custom path"
                                  LIBASS_LIBRARY LIBASS_INCLUDE_DIR)

mark_as_advanced(LIBASS_INCLUDE_DIR LIBASS_LIBRARY )
