
cmake_minimum_required (VERSION 2.8)


# set a default build type if none was provided
# this has to be done before the project() instruction!
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

project (sfeMovie)

set (CMAKE_MODULE_PATH "./cmake/Modules")

# detect the OS
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

set (BUILD_WITH_PROVIDED_FFMPEG TRUE CACHE BOOL "TRUE to build sfeMovie with the provided FFmpeg libraries, FALSE to build with the system libraries")
set (BUILD_SFEMOVIE_SAMPLE FALSE CACHE BOOL "TRUE to build the sfeMovie sample")

# Set some env vars
set (LIB_NAME sfeMovie)
set (VERSION_MAJOR 1)
set (VERSION_MINOR 0)
set (VERSION_PATCH 1)
set (COMPILER_FLAGS "")
set (LINKER_FLAGS "")

set (SOURCES_DIR "src")
set (PUBLIC_HEADERS "include")
set (PRIVATE_HEADERS "src")
set (OTHER_LIBRARIES "")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_definitions("-O2")
endif()
	
if (BUILD_WITH_PROVIDED_FFMPEG)
    set (PRIVATE_HEADERS ${PRIVATE_HEADERS} "deps/headers")
    set (FFMPEG_LIBRARIES avdevice avformat avcodec avutil swscale)
    set (EXTERNAL_LIBRARIES_DIR ${CMAKE_SOURCE_DIR}/deps/ffmpeg-build)
endif()

set (SOURCE_FILES ${SOURCES_DIR}/Movie.cpp ${SOURCES_DIR}/Movie_audio.cpp ${SOURCES_DIR}/Movie_video.cpp ${SOURCES_DIR}/utils.cpp ${SOURCES_DIR}/Condition.cpp)

find_package (SFML 2 COMPONENTS graphics window system audio REQUIRED)
set(SFML_LIBRARIES ${SFML_GRAPHICS_LIBRARY} ${SFML_WINDOW_LIBRARY} ${SFML_AUDIO_LIBRARY} ${SFML_SYSTEM_LIBRARY})


if (LINUX) # ========================================== LINUX ========================================== #

    if (NOT ${BUILD_WITH_PROVIDED_FFMPEG})
        find_package (FFMPEG REQUIRED)
	endif()
	
    add_definitions(-D__STDC_CONSTANT_MACROS)
	set (SOURCE_FILES ${SOURCE_FILES} "${SOURCES_DIR}/Unix/ConditionImpl.cpp")

elseif (MACOSX) # ========================================== MACOSX ========================================== #
	
	if (HAS_VDA)
		set (OTHER_LIBRARIES ${OTHER_LIBRARIES} z bz2 "-framework VideoDecodeAcceleration" "-framework CoreVideo")
	else()
		# In case VDA isn't defined (ie. build.sh is using the current OS SDK), check the current OS version
		if (DEFINED HAS_VDA) # HAS_VDA defined but FALSE
			set (OTHER_LIBRARIES ${OTHER_LIBRARIES} z bz2)
		else()
			# VDA is used for OS X 10.7 and later
			if (${CMAKE_HOST_SYSTEM_VERSION} VERSION_EQUAL "11.0.0" OR ${CMAKE_HOST_SYSTEM_VERSION} VERSION_GREATER "11.0.0")
				set (OTHER_LIBRARIES ${OTHER_LIBRARIES} z bz2 "-framework VideoDecodeAcceleration" "-framework CoreVideo")
			endif()
		endif()
	endif()
	
	set (SOURCE_FILES ${SOURCE_FILES} "${SOURCES_DIR}/Unix/ConditionImpl.cpp")
	set (CMAKE_OSX_ARCHITECTURES "x86_64")
    
    # add an option to let the user specify a custom directory for framework installation
    set(CMAKE_INSTALL_FRAMEWORK_PREFIX "/Library/Frameworks" CACHE STRING "Frameworks installation directory")
	
	# find only "root" headers
    file(GLOB SFE_HEADERS RELATIVE ${PROJECT_SOURCE_DIR} "include/sfeMovie/*")
    
    # Make sure sfeMovie headers get imported in the framework
    set (SOURCE_FILES ${SOURCE_FILES} ${SFE_HEADERS})
	
	
elseif (WINDOWS) # ========================================== WINDOWS ========================================== #

	set (EXTERNAL_LIBRARIES_DIR ${EXTERNAL_LIBRARIES_DIR} "${CMAKE_SOURCE_DIR}/deps/windows-binaries")
	
	if (MSVC)
		set (PRIVATE_HEADERS ${PRIVATE_HEADERS} "${CMAKE_SOURCE_DIR}/deps/headers/msvc")
	else()
		set (EXTERNAL_LIBRARIES_DIR ${EXTERNAL_LIBRARIES_DIR} "${CMAKE_SOURCE_DIR}/deps/windows-binaries/gcc")
		set (LINKER_FLAGS ${LINKER_FLAGS} "-static-libgcc")
		set (OTHER_LIBRARIES ${OTHER_LIBRARIES} "z")
	endif()
	
	add_definitions(-D__STDC_CONSTANT_MACROS -DSFE_EXPORTS)
	set (OTHER_LIBRARIES ${OTHER_LIBRARIES} "ws2_32")
	set (SOURCE_FILES ${SOURCE_FILES} "${SOURCES_DIR}/Win32/ConditionImpl.cpp")
endif()



if (${BUILD_WITH_PROVIDED_FFMPEG})
    include_directories (${SFML_INCLUDE_DIR} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
    link_directories (${FFMPEG_LIBRARY_DIR})
else ()
    # Set the search directories for compiling and link editing
    include_directories (${SFML_INCLUDE_DIR} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS} ${FFMPEG_INCLUDE_DIR})
    link_directories (${EXTERNAL_LIBRARIES_DIR})
endif ()

# Define the source files sfe should compile
add_library (${LIB_NAME} SHARED ${SOURCE_FILES})


set_target_properties(${LIB_NAME} PROPERTIES
					  COMPILE_FLAGS "${COMPILER_FLAGS}"
					  LINK_FLAGS "${LINKER_FLAGS}"
					  SOVERSION "${VERSION_MAJOR}.${VERSION_MINOR}.0"
                      VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")


if (LINUX)
	install(DIRECTORY include/sfeMovie
            DESTINATION ${CMAKE_INSTALL_PREFIX}/include
            COMPONENT devel)
    		
    install(TARGETS ${LIB_NAME}
            LIBRARY DESTINATION lib)
   	
   	file(GLOB FFMPEG_BUILD_LIBS RELATIVE ${PROJECT_SOURCE_DIR} "deps/ffmpeg-build/*")
    install(FILES ${FFMPEG_BUILD_LIBS}
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
            COMPONENT devel)
elseif (MACOSX)
    # edit target properties
    set_target_properties(${LIB_NAME} PROPERTIES 
                          FRAMEWORK TRUE
                          FRAMEWORK_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                          MACOSX_FRAMEWORK_IDENTIFIER org.yalir.${LIB_NAME}
                          MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                          MACOSX_FRAMEWORK_BUNDLE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                          PUBLIC_HEADER ${SFE_HEADERS} )


    # adapt install directory to allow distributing dylibs/frameworks in user’s frameworks/application bundle
    set_target_properties(${LIB_NAME} PROPERTIES 
                          BUILD_WITH_INSTALL_RPATH 1 
                          INSTALL_NAME_DIR "@executable_path/../Frameworks")
    
    install(TARGETS ${LIB_NAME}
            FRAMEWORK DESTINATION ${CMAKE_INSTALL_FRAMEWORK_PREFIX}
            COMPONENT devel)
endif()


# Define the libraries sfe should link against
target_link_libraries (${LIB_NAME} ${SFML_LIBRARIES} ${FFMPEG_LIBRARIES} ${OTHER_LIBRARIES})

if (BUILD_SFEMOVIE_SAMPLE)
    add_subdirectory(sample)
endif ()

