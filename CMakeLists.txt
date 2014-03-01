
cmake_minimum_required (VERSION 2.8)


# set a default build type if none was provided
# this has to be done before the project() instruction!
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

project (sfeMovie)
include(CTest)

# Import required CMake files
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")
include(RunShellCommand.cmake)

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

set (LINK_AGAINST_INTERNAL_FFMPEG TRUE CACHE BOOL "TRUE to build sfeMovie with the provided FFmpeg sources, FALSE to build with the system libraries")
set (BUILD_SFEMOVIE_SAMPLE FALSE CACHE BOOL "TRUE to build the sfeMovie sample")
set (BUILD_DEMUXER_CLI TRUE CACHE BOOL "TRUE to build the Demuxer command line program (to check file contents and play audio)")

# Set some env vars
set (LIB_NAME sfeMovie)
set (VERSION_MAJOR 2)
set (VERSION_MINOR 0)
set (COMPILER_FLAGS "")
set (LINKER_FLAGS "")

set (SOURCES_DIR "src")
set (PUBLIC_HEADERS "include")
set (PRIVATE_HEADERS "src")
set (OTHER_LIBRARIES "")
set (EXTERNAL_LIBRARIES_DIR "")
set (BUILTIN_FFMPEG_BUILD_DIR "${CMAKE_BINARY_DIR}/FFmpeg-binaries")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_definitions("-O2")
endif()

# These variables will be used by build_ffmpeg.sh. We don't give them
# as parameters to the script because RunShell() doesn't allow parameters
# with spaces on Windows, thus we use a portable solution
file(WRITE "${CMAKE_SOURCE_DIR}/SourceDir.var" "${CMAKE_SOURCE_DIR}")
file(WRITE "${CMAKE_SOURCE_DIR}/BuildDir.var" "${CMAKE_BINARY_DIR}")

#################################################################################################################
# ============================================== FFMPEG SETUP ================================================= #
#################################################################################################################

# Set ${output} to the full path of the given library (depends on platform and on dynamic/static settings)
macro(import_library_path output basePath libName)
	if (MSVC)
		set(SUFFIX ".lib")
	else()
		set(SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
	endif()
	set(${output} "${basePath}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}${libName}${SUFFIX}")
endmacro(import_library_path)

# Define ${FFMPEG_LIBRARIES} to the paths of all the required FFmpeg libraries
macro(ffmpeg_paths search_path)
	import_library_path(FFMPEG_LIBAVFORMAT_LIBRARIES "${search_path}" "avformat")
	import_library_path(FFMPEG_LIBAVDEVICE_LIBRARIES "${search_path}" "avdevice")
	import_library_path(FFMPEG_LIBAVCODEC_LIBRARIES "${search_path}" "avcodec")
	import_library_path(FFMPEG_LIBAVUTIL_LIBRARIES "${search_path}" "avutil")
	import_library_path(FFMPEG_LIBSWSCALE_LIBRARIES "${search_path}" "swscale")
	import_library_path(FFMPEG_LIBSWRESAMPLE_LIBRARIES "${search_path}" "swresample")
	
	set (FFMPEG_LIBRARIES
			${FFMPEG_LIBAVFORMAT_LIBRARIES}
			${FFMPEG_LIBAVDEVICE_LIBRARIES}
			${FFMPEG_LIBAVCODEC_LIBRARIES}
			${FFMPEG_LIBAVUTIL_LIBRARIES}
			${FFMPEG_LIBSWSCALE_LIBRARIES}
			${FFMPEG_LIBSWRESAMPLE_LIBRARIES})
endmacro(ffmpeg_paths)

# Check that all of the FFmpeg headers can be found in ${FFMPEG_INCLUDE_DIR}
macro(check_ffmpeg_headers)
	foreach(header "libavcodec/avcodec.h" "libavdevice/avdevice.h" "libavformat/avformat.h" "libavutil/avutil.h" "libswscale/swscale.h" "libswresample/swresample.h")
		if(NOT EXISTS "${FFMPEG_INCLUDE_DIR}/${header}")
			message(FATAL_ERROR "The chosen FFmpeg is missing a header file: ${header}")
		endif()
	endforeach()
endmacro(check_ffmpeg_headers)

# Check that all of the FFmpeg libraries can be found in ${FFMPEG_LIBRARIES}
macro(check_ffmpeg_libraries)
	foreach(lib ${FFMPEG_LIBRARIES})
		if(NOT EXISTS "${lib}")
			if(${LINK_AGAINST_INTERNAL_FFMPEG})
				message(WARNING "You chose to link sfeMovie against the provided FFmpeg but you also chose not to build the provided FFmpeg. However, the builtin FFmpeg has not been built yet and the FFmpeg binaries are missing")
			endif()
			message(FATAL_ERROR "The chosen FFmpeg is missing a library file: ${lib}")
		endif()
	endforeach()
endmacro(check_ffmpeg_libraries)

# Setup the paths that will be used for linking against FFmpeg and eventually building it
if (LINK_AGAINST_INTERNAL_FFMPEG)
	# Use our own FFmpeg libraries
	set (FFMPEG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/headers")
    set (FFMPEG_LIBRARY_DIR "${BUILTIN_FFMPEG_BUILD_DIR}")
    ffmpeg_paths("${BUILTIN_FFMPEG_BUILD_DIR}")
    check_ffmpeg_headers()
else()
	# Use user's FFmpeg libraries
	set (FFMPEG_INCLUDE_DIR "" CACHE PATH "Path to the directory containing the FFmpeg headers directories (libavcodec, libavformat, etc ; leave empty to let CMake find them)")
	set (FFMPEG_LIBRARY_DIR "" CACHE PATH "Path to the directory containing the FFmpeg libraries (leave empty to let CMake find them)")
	
	if ("${FFMPEG_INCLUDE_DIR}" STREQUAL "" OR "${FFMPEG_LIBRARY_DIR}" STREQUAL "")
		find_package (FFMPEG REQUIRED)
		set(FFMPEG_INCLUDE_DIR "${FFMPEG_LIBAVCODEC_INCLUDE_DIRS}" CACHE PATH "Path to the directory containing the FFmpeg headers directories (libavcodec, libavformat, etc ; leave empty to let CMake find them)" FORCE)

		if ("${FFMPEG_FOUND}" STREQUAL "NO")
			message(FATAL_ERROR "Could not find a full FFmpeg installation")
		endif()
	else()
		ffmpeg_paths("${FFMPEG_LIBRARY_DIR}")
		check_ffmpeg_headers()
		check_ffmpeg_libraries()
	endif()
endif()

message(STATUS "Using these FFmpeg libraries:")
foreach(library ${FFMPEG_LIBRARIES})
	message(STATUS "- ${library}")
endforeach()


#################################################################################################################
# ================================================ SFML SETUP ================================================= #
#################################################################################################################

find_package (SFML 2 COMPONENTS graphics window system audio REQUIRED)
set(SFML_LIBRARIES ${SFML_GRAPHICS_LIBRARY} ${SFML_WINDOW_LIBRARY} ${SFML_AUDIO_LIBRARY} ${SFML_SYSTEM_LIBRARY})

#################################################################################################################
# =============================================== Boost SETUP ================================================= #
#################################################################################################################

set(Boost_USE_STATIC_LIBS ON)
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
	if (MSVC)
		set (PRIVATE_HEADERS ${PRIVATE_HEADERS} "${CMAKE_SOURCE_DIR}/deps/headers/msvc")
	else()
		set (EXTERNAL_LIBRARIES_DIR ${EXTERNAL_LIBRARIES_DIR} "${CMAKE_SOURCE_DIR}/deps/windows-binaries/gcc")
		set (LINKER_FLAGS ${LINKER_FLAGS} "-static-libgcc")
		set (OTHER_LIBRARIES ${OTHER_LIBRARIES} "z") # TODO: use find_library() and discard EXTERNAL_LIBRARIES_DIR
	endif()
	
	add_definitions(-D__STDC_CONSTANT_MACROS -DSFE_EXPORTS)
	set (OTHER_LIBRARIES ${OTHER_LIBRARIES} "ws2_32")
	set (SOURCE_FILES ${SOURCE_FILES} "${SOURCES_DIR}/Win32/ConditionImpl.cpp")
endif()

source_group(Sources FILES ${SOURCE_FILES})

# Set the search directories for compiling and link editing
include_directories (${SFML_INCLUDE_DIR} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS} ${FFMPEG_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})
link_directories (${EXTERNAL_LIBRARIES_DIR})

message (STATUS "Headers directories: ${SFML_INCLUDE_DIR} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS} ${FFMPEG_INCLUDE_DIR} ${Boost_INCLUDE_DIRS}")
message (STATUS "Libraries directories: ${EXTERNAL_LIBRARIES_DIR}")

# Build FFmpeg if needed
if (${LINK_AGAINST_INTERNAL_FFMPEG})
	if (WINDOWS)
		set( MINGW_DIR C:/MinGW CACHE PATH "Set the path for the MinGW environment")
		if (EXISTS "${MINGW_DIR}" AND IS_DIRECTORY "${MINGW_DIR}")
			if (EXISTS "${MINGW_DIR}/bin" AND IS_DIRECTORY "${MINGW_DIR}/bin")
				if (NOT EXISTS "${MINGW_DIR}/msys/1.0/bin" OR NOT IS_DIRECTORY "${MINGW_DIR}/msys/1.0/bin")
					message(FATAL_ERROR "The specified MINGW_DIR does not contain MSYS. You're most probably using CodeBlock's MinGW, but sfeMovie also needs a bash interpreter that MSYS provides. Please download and install a full MinGW + MSYS system.")
				endif()
			else()
				message(FATAL_ERROR "The specified MINGW_DIR is not a MinGW directory")
			endif()
		else()
			message(FATAL_ERROR "The specified MINGW_DIR is not a valid path to a directory")
		endif()
	endif()

	set (Decoders_that_you_can_enable aac aac_latm aasc ac3 adpcm_4xm adpcm_adx adpcm_afc adpcm_ct adpcm_dtk
	  adpcm_ea adpcm_ea_maxis_xa adpcm_ea_r1 adpcm_ea_r2 adpcm_ea_r3 adpcm_ea_xas adpcm_g722 adpcm_g726
	  adpcm_g726le adpcm_ima_amv adpcm_ima_apc adpcm_ima_dk3 adpcm_ima_dk4 adpcm_ima_ea_eacs adpcm_ima_ea_sead
	  adpcm_ima_iss adpcm_ima_oki adpcm_ima_qt adpcm_ima_rad adpcm_ima_smjpeg adpcm_ima_wav adpcm_ima_ws adpcm_ms
	  adpcm_sbpro_2 adpcm_sbpro_3 adpcm_sbpro_4 adpcm_swf adpcm_thp adpcm_xa adpcm_yamaha aic alac als amrnb
	  amrwb amv anm ansi ape ass asv1 asv2 atrac1 atrac3 atrac3p aura aura2 avrn avrp avs avui ayuv bethsoftvid
	  bfi bink binkaudio_dct binkaudio_rdft bintext bmp bmv_audio bmv_video brender_pix c93 cavs cdgraphics cdxl
	  cinepak cljr cllc comfortnoise cook cpia cscd cyuv dca dfa dirac dnxhd dpx dsicinaudio dsicinvideo dvbsub
	  dvdsub dvvideo dxa dxtory eac3 eacmv eamad eatgq eatgv eatqi eightbps eightsvx_exp eightsvx_fib escape124
	  escape130 evrc exr ffv1 ffvhuff ffwavesynth fic flac flashsv flashsv2 flic flv fourxm fraps frwu g2m g723_1
	  g729 gif gsm gsm_ms h261 h263 h263i h263p h264 h264_crystalhd h264_vda h264_vdpau hevc hnm4_video huffyuv
	  iac idcin idf iff_byterun1 iff_ilbm imc indeo2 indeo3 indeo4 indeo5 interplay_dpcm interplay_video jacosub
	  jpeg2000 jpegls jv kgv1 kmvc lagarith libcelt libfdk_aac libgsm libgsm_ms libilbc libopencore_amrnb
	  libopencore_amrwb libopenjpeg libopus libschroedinger libspeex libstagefright_h264 libutvideo libvorbis
	  libvpx_vp8 libvpx_vp9 libzvbi_teletext loco mace3 mace6 mdec metasound microdvd mimic mjpeg mjpegb mlp
	  mmvideo motionpixels movtext mp1 mp1float mp2 mp2float mp3 mp3adu mp3adufloat mp3float mp3on4 mp3on4float
	  mpc7 mpc8 mpeg1_vdpau mpeg1video mpeg2_crystalhd mpeg2video mpeg4 mpeg4_crystalhd mpeg4_vdpau mpeg_vdpau
	  mpeg_xvmc mpegvideo mpl2 msa1 msmpeg4_crystalhd msmpeg4v1 msmpeg4v2 msmpeg4v3 msrle mss1 mss2 msvideo1 mszh
	  mts2 mvc1 mvc2 mxpeg nellymoser nuv paf_audio paf_video pam pbm pcm_alaw pcm_bluray pcm_dvd pcm_f32be
	  pcm_f32le pcm_f64be pcm_f64le pcm_lxf pcm_mulaw pcm_s16be pcm_s16be_planar pcm_s16le pcm_s16le_planar
	  pcm_s24be pcm_s24daud pcm_s24le pcm_s24le_planar pcm_s32be pcm_s32le pcm_s32le_planar pcm_s8 pcm_s8_planar
	  pcm_u16be pcm_u16le pcm_u24be pcm_u24le pcm_u32be pcm_u32le pcm_u8 pcm_zork pcx pgm pgmyuv pgssub pictor
	  pjs png ppm prores prores_lgpl ptx qcelp qdm2 qdraw qpeg qtrle r10k r210 ra_144 ra_288 ralf rawvideo
	  realtext rl2 roq roq_dpcm rpza rv10 rv20 rv30 rv40 s302m sami sanm sgi sgirle shorten sipr smackaud smacker
	  smc smvjpeg snow sol_dpcm sonic sp5x srt ssa subrip subviewer subviewer1 sunrast svq1 svq3 tak targa
	  targa_y216 text theora thp tiertexseqvideo tiff tmv truehd truemotion1 truemotion2 truespeech tscc tscc2
	  tta twinvq txd ulti utvideo v210 v210x v308 v408 v410 vb vble vc1 vc1_crystalhd vc1_vdpau vc1image vcr1
	  vima vmdaudio vmdvideo vmnc vorbis vp3 vp5 vp6 vp6a vp6f vp8 vp9 vplayer vqa wavpack webp webvtt
	  wmalossless wmapro wmav1 wmav2 wmavoice wmv1 wmv2 wmv3 wmv3_crystalhd wmv3_vdpau wmv3image wnv1 ws_snd1
	  xan_dpcm xan_wc3 xan_wc4 xbin xbm xface xl xsub xwd y41p yop yuv4 zero12v zerocodec zlib zmbv
	  CACHE STRING "Available decoders" FORCE)
	set (PCM_DECODERS adpcm_4xm adpcm_adx adpcm_afc adpcm_ct adpcm_dtk adpcm_ea adpcm_ea_maxis_xa adpcm_ea_r1
	  adpcm_ea_r2 adpcm_ea_r3 adpcm_ea_xas adpcm_g722 adpcm_g726 adpcm_g726le adpcm_ima_amv adpcm_ima_apc
	  adpcm_ima_dk3 adpcm_ima_dk4 adpcm_ima_ea_eacs adpcm_ima_ea_sead adpcm_ima_iss adpcm_ima_oki adpcm_ima_qt
	  adpcm_ima_rad adpcm_ima_smjpeg adpcm_ima_wav adpcm_ima_ws adpcm_ms adpcm_sbpro_2 adpcm_sbpro_3
	  adpcm_sbpro_4 adpcm_swf adpcm_thp adpcm_xa adpcm_yamaha interplay_dpcm pcm_alaw pcm_bluray pcm_dvd
	  pcm_f32be pcm_f32le pcm_f64be pcm_f64le pcm_lxf pcm_mulaw pcm_s16be pcm_s16be_planar pcm_s16le
	  pcm_s16le_planar pcm_s24be pcm_s24daud pcm_s24le pcm_s24le_planar pcm_s32be pcm_s32le pcm_s32le_planar
	  pcm_s8 pcm_s8_planar pcm_u16be pcm_u16le pcm_u24be pcm_u24le pcm_u32be pcm_u32le pcm_u8 pcm_zork roq_dpcm
	  sol_dpcm xan_dpcm)
	set (ENABLED_DECODERS theora flac vorbis vp8 ${PCM_DECODERS} CACHE STRING "Enabled audio and video decoders")
	
	if (MACOSX)
		set(OS_ARG "macosx")
	elseif(WINDOWS)
		set(OS_ARG "windows")
	elseif(LINUX)
		set(OS_ARG "linux")
	endif()
	
	if (MSVC)
		set(VS_ARG "vs")
	else()
		set(VS_ARG "novs")
	endif()
	
	if (MACOSX)
		set(ARCH_ARG ${CMAKE_OSX_ARCHITECTURES})
	else()
		set(ARCH_ARG notosx)
	endif()
	
	RunShell(${LIB_NAME} PRE_LINK ./build_ffmpeg.sh ${OS_ARG} ${VS_ARG} ${ARCH_ARG} ${ENABLED_DECODERS})
endif()

# Define the source files sfe should compile
add_library (${LIB_NAME} SHARED ${SOURCE_FILES})
add_dependencies(${LIB_NAME} FFmpeg)

if (${LINK_AGAINST_INTERNAL_FFMPEG})
	add_custom_command(TARGET ${LIB_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${CMAKE_BINARY_DIR}/FFmpeg-binaries/lib"
			${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
endif()


add_subdirectory(tests)

set_target_properties(${LIB_NAME} PROPERTIES
					  DEBUG_POSTFIX "-d"
					  COMPILE_FLAGS "${COMPILER_FLAGS}"
					  LINK_FLAGS "${LINKER_FLAGS}"
					  SOVERSION "${VERSION_MAJOR}.${VERSION_MINOR}.0"
                      VERSION "${VERSION_MAJOR}.${VERSION_MINOR}")


if (MACOSX)
    # edit target properties
    set_target_properties(${LIB_NAME} PROPERTIES 
                          FRAMEWORK TRUE
                          FRAMEWORK_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}
                          MACOSX_FRAMEWORK_IDENTIFIER org.yalir.${LIB_NAME}
                          MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}
                          MACOSX_FRAMEWORK_BUNDLE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}
                          PUBLIC_HEADER ${SFE_HEADERS} )


    # adapt install directory to allow distributing dylibs/frameworks in user’s frameworks/application bundle
    set_target_properties(${LIB_NAME} PROPERTIES 
                          BUILD_WITH_INSTALL_RPATH 1 
                          INSTALL_NAME_DIR "@rpath")
endif()

# Install sfeMovie library and headers
if (NOT MACOSX)
	install(DIRECTORY include
			DESTINATION .)

	install(TARGETS ${LIB_NAME}
        RUNTIME DESTINATION bin COMPONENT bin
        LIBRARY DESTINATION lib${LIB_SUFFIX} COMPONENT bin 
        ARCHIVE DESTINATION lib${LIB_SUFFIX} COMPONENT devel)
else()
	install(TARGETS ${LIB_NAME}
            FRAMEWORK DESTINATION ${CMAKE_INSTALL_FRAMEWORK_PREFIX}
            COMPONENT bin)
endif()

# Install FFmpeg
if (LINK_AGAINST_INTERNAL_FFMPEG)
	if (NOT MACOSX)
		if (MSVC)
			install(DIRECTORY "${BUILTIN_FFMPEG_BUILD_DIR}/bin" DESTINATION "${CMAKE_INSTALL_PREFIX}"
				FILES_MATCHING PATTERN "*-*")
		else()
			install(DIRECTORY "${BUILTIN_FFMPEG_BUILD_DIR}/bin" DESTINATION "${CMAKE_INSTALL_PREFIX}")
		endif()
	endif()
	
	if (LINUX OR (WINDOWS AND NOT MSVC))
		install(DIRECTORY "${BUILTIN_FFMPEG_BUILD_DIR}/lib" DESTINATION "${CMAKE_INSTALL_PREFIX}")
	endif()

	# Linux and MSVC use FFmpeg's dynamic library (instead of static FFmpeg libraries)
	# if (MSVC OR LINUX)
	# 	if (MSVC)
	# 		set (FFMPEG_INSTALL_DIR "bin")
	# 	else ()
	# 		set (FFMPEG_INSTALL_DIR "lib")
	# 	endif()

	#     install(FILES ${FFMPEG_LIBRARIES}
	#             DESTINATION ${CMAKE_INSTALL_PREFIX}/${FFMPEG_INSTALL_DIR}
	#             COMPONENT devel)
	# endif()
endif()

# Define the libraries sfe should link against
target_link_libraries (${LIB_NAME} ${SFML_LIBRARIES} ${FFMPEG_LIBRARIES} ${OTHER_LIBRARIES} ${Boost_LIBRARIES})

# Sample building
if (BUILD_SFEMOVIE_SAMPLE)
    add_subdirectory(sample)
endif ()

# Demuxer command line tool
if (BUILD_DEMUXER_CLI)
	add_subdirectory(DemuxerCLI)
endif()

# add an option for building the documentation
set(BUILD_DOC FALSE CACHE BOOL "Set to true to build the documentation")

if(BUILD_DOC)
    add_subdirectory(doxygen)
endif()
