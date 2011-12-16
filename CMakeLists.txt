
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
set (VERSION_MAJOR 1)
set (VERSION_MINOR 0)
set (VERSION_PATCH 0)

set (SOURCES "src")
set (PUBLIC_HEADERS "include")
set (PRIVATE_HEADERS "src")
set (EXTERNAL_HEADERS "deps/headers")
set (FFMPEG_LIBRARIES avdevice avformat avcodec avutil swscale)
set (OTHER_LIBRARIES "")
set (SFML_LIBRARIES "")
set (COMPILER_FLAGS "-g2")
set (LINKER_FLAGS "")
set (OTHER_FLAGS "")

set (SOURCE_FILES ${SOURCES}/Movie.cpp ${SOURCES}/Movie_audio.cpp ${SOURCES}/Movie_video.cpp ${SOURCES}/utils.cpp ${SOURCES}/Condition.cpp)

if (LINUX)
	set (EXTERNAL_LIBRARIES "${CMAKE_SOURCE_DIR}/deps/linux-binaries" "${CMAKE_SOURCE_DIR}/deps/ffmpeg-build")
	set (SOURCE_FILES ${SOURCE_FILES} ${SOURCES}/Unix/ConditionImpl.cpp)
	set (COMPILER_FLAGS "${COMPILER_FLAGS} -D__STDC_CONSTANT_MACROS")
	set (SFML_LIBRARIES sfml-system.2.0 sfml-window.2.0 sfml-graphics.2.0 sfml-audio.2.0)
	
elseif (MACOSX)
	set (EXTERNAL_LIBRARIES "${CMAKE_SOURCE_DIR}/deps/macosx-binaries" "${CMAKE_SOURCE_DIR}/deps/ffmpeg-build")
	set (OTHER_LIBRARIES z bz2)
	set (LINKER_FLAGS "${LINKER_FLAGS} -read_only_relocs suppress -framework OpenGL")
	set (LINKER_FLAGS "${LINKER_FLAGS} -framework sfml-system -framework sfml-window -framework sfml-graphics -framework sfml-audio")
	set (SOURCE_FILES ${SOURCE_FILES} ${SOURCES}/Unix/ConditionImpl.cpp)
	
	# arch support - by default : i386
    #if(NOT CMAKE_OSX_ARCHITECTURES)
    #    set(CMAKE_OSX_ARCHITECTURES "i386" CACHE STRING "Build architectures for Mac OS X" FORCE)
    #endif()
	set (CMAKE_OSX_ARCHITECTURES "i386")
	
	if(EXISTS /Developer/SDKs/MacOSX10.5.sdk)
        # target 10.5 system
        set(CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.5.sdk"
            CACHE STRING "The product will be built against the headers and libraries located inside the indicated SDK. Set to empty string for default value."
            FORCE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.5" 
            CACHE STRING "Minimum OS X version to target for deployment (at runtime); newer APIs weak linked."
            FORCE)
    elseif(EXISTS /Developer/SDKs/MacOSX10.6.sdk)
        # target 10.6 system
        set(CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.6.sdk"
            CACHE STRING "The product will be built against the headers and libraries located inside the indicated SDK. Set to empty string for default value."
            FORCE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.6" 
            CACHE STRING "Minimum OS X version to target for deployment (at runtime); newer APIs weak linked."
            FORCE)
    else()
        # use default SDK.
    endif()
    
    # add an option to let the user specify a custom directory for frameworks installation (SFML, sndfile, ...)
    set(CMAKE_INSTALL_FRAMEWORK_PREFIX "/Library/Frameworks" CACHE STRING "Frameworks installation directory")
	
	# find only "root" headers
    file(GLOB SFE_HEADERS RELATIVE ${PROJECT_SOURCE_DIR} "include/*")
    
    set (SOURCE_FILES ${SOURCE_FILES} ${SFE_HEADERS})
	
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
	set (OTHER_LIBRARIES ${OTHER_LIBRARIES} ws2_32 opengl32)
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

if (MACOSX)
    # edit target properties
    set_target_properties(${LIB_NAME} PROPERTIES 
                          FRAMEWORK TRUE
                          FRAMEWORK_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                          SOVERSION ${VERSION_MAJOR}.${VERSION_MINOR}.0
                          VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                          MACOSX_FRAMEWORK_IDENTIFIER org.yalir.${LIB_NAME}
                          MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                          MACOSX_FRAMEWORK_BUNDLE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                          PUBLIC_HEADER "${SFE_HEADERS}" )

    # adapt install directory to allow distributing dylibs/frameworks in user’s frameworks/application bundle
    set_target_properties(${LIB_NAME} PROPERTIES 
                          BUILD_WITH_INSTALL_RPATH 1 
                          INSTALL_NAME_DIR "@executable_path/../Frameworks")
endif()


# Define the libraries sfe should link against
target_link_libraries (${LIB_NAME} ${SFML_LIBRARIES} ${FFMPEG_LIBRARIES} ${OTHER_LIBRARIES})

