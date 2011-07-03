
cmake_minimum_required (VERSION 2.8)


# set a default build type if none was provided
# this has to be done before the project() instruction!
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

project (sfeMovie)

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


# Set some env vars
set (LIB_NAME sfe-movie)

set (SOURCES "src")
set (PUBLIC_HEADERS "include")
set (PRIVATE_HEADERS "src")
set (EXTERNAL_HEADERS "deps/headers")
set (SFML_LIBRARIES sfml-system.2.0 sfml-window.2.0 sfml-graphics.2.0 sfml-audio.2.0)
set (FFMPEG_LIBRARIES avdevice avformat avcodec avutil swscale)
set (OTHER_LIBRARIES "")

set (COMPILER_FLAGS "")
set (LINKER_FLAGS "")
set (OTHER_FLAGS "")

set (SOURCE_FILES ${SOURCES}/Movie.cpp ${SOURCES}/Movie_audio.cpp ${SOURCES}/Movie_video.cpp ${SOURCES}/utils.cpp ${SOURCES}/Condition.cpp)

if (LINUX)
	set (EXTERNAL_LIBRARIES "${CMAKE_SOURCE_DIR}/deps/linux-binaries" "${CMAKE_SOURCE_DIR}/deps/ffmpeg-build")
	set (SOURCE_FILES ${SOURCE_FILES} ${SOURCES}/Unix/ConditionImpl.cpp)
	set (COMPILER_FLAGS "${COMPILER_FLAGS} -D__STDC_CONSTANT_MACROS")
	
elseif (MACOSX)
	set (EXTERNAL_LIBRARIES "${CMAKE_SOURCE_DIR}/deps/macosx-binaries" "${CMAKE_SOURCE_DIR}/deps/ffmpeg-build")
	set (OTHER_LIBRARIES z bz2)
	
#	set (COMPILER_FLAGS "${COMPILER_FLAGS} -g2")
	set (LINKER_FLAGS "${LINKER_FLAGS} -read_only_relocs suppress")
	
	# add " -arch i386 -arch x86_64" to $OTHER_FLAGS in order to build universal binary
	# (disabled by default because I couldn't find any way to get FFmpeg to build as
	# universal binary when being built from build.sh)
	#set (OTHER_FLAGS "${OTHER_FLAGS} --sysroot=/Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5 -arch i386")
	set (CMAKE_OSX_ARCHITECTURES "i386")
	set (CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.5.sdk")
    set (CMAKE_OSX_DEPLOYMENT_TARGET "10.5")
	set (SOURCE_FILES ${SOURCE_FILES} ${SOURCES}/Unix/ConditionImpl.cpp)
	
elseif (WINDOWS)
	set (EXTERNAL_LIBRARIES ${CMAKE_SOURCE_DIR}/deps/windows-binaries ${CMAKE_SOURCE_DIR}/deps/ffmpeg-build)
	
	if (MSVC)
		set (EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${CMAKE_SOURCE_DIR}/deps/windows-binaries/msvc)
		set (PRIVATE_HEADERS ${PRIVATE_HEADERS} ${CMAKE_SOURCE_DIR}/deps/headers/msvc)
	else()
		set (EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${CMAKE_SOURCE_DIR}/deps/windows-binaries/gcc)
		set (LINKER_FLAGS "${LINKER_FLAGS} -static-libgcc -static-libstdc++")
		set (OTHER_LIBRARIES ${OTHER_LIBRARIES} z)
	endif()
	
	set (COMPILER_FLAGS "${COMPILER_FLAGS} -D__STDC_CONSTANT_MACROS -DSFE_EXPORTS")
	set (SFML_LIBRARIES sfml-system sfml-window sfml-graphics sfml-audio)
	set (OTHER_LIBRARIES ${OTHER_LIBRARIES} ws2_32)
	set (SOURCE_FILES ${SOURCE_FILES} ${SOURCES}/Win32/ConditionImpl.cpp)
endif()


# Set the search directories for compiling and link editing
include_directories (${PUBLIC_HEADERS})
include_directories (${PRIVATE_HEADERS})
include_directories (${EXTERNAL_HEADERS})

link_directories (${EXTERNAL_LIBRARIES})

# Define the source files sfe should compile
add_library (${LIB_NAME} SHARED ${SOURCE_FILES})


set_target_properties(${LIB_NAME} PROPERTIES COMPILE_FLAGS "${COMPILER_FLAGS} ${OTHER_FLAGS}")
set_target_properties(${LIB_NAME} PROPERTIES LINK_FLAGS "${LINKER_FLAGS} ${OTHER_FLAGS}")

# Set install name
if (MACOSX)
	set_target_properties(${LIB_NAME} PROPERTIES
	   BUILD_WITH_INSTALL_RPATH 1
	   INSTALL_NAME_DIR "@executable_path/../Frameworks")
endif()


# Define the libraries sfe should link against
if (MSVC)
	target_link_libraries (${LIB_NAME} ${SFML_LIBRARIES} ${OTHER_LIBRARIES})
else()
	target_link_libraries (${LIB_NAME} ${SFML_LIBRARIES} ${FFMPEG_LIBRARIES} ${OTHER_LIBRARIES})
endif()

INSTALL(FILES lib${LIB_NAME}.dylib
	DESTINATION /usr/local/lib)

INSTALL(FILES ${PUBLIC_HEADERS}/Movie.h
	DESTINATION /usr/local/include/sfe)
