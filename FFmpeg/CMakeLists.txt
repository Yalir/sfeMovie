
# Setup the paths that will be used for linking against FFmpeg and eventually building it
if (NOT LINK_AGAINST_INTERNAL_FFMPEG)
	# Use user's FFmpeg libraries
	find_package(FFMPEG REQUIRED)
	if ("${FFMPEG_FOUND}" STREQUAL "NO")
		message(FATAL_ERROR "Could not find a full FFmpeg installation")
	endif()
else()
	# These variables will be used by build_ffmpeg.sh. We don't give them
	# as parameters to the script because add_shell_command() doesn't allow parameters
	# with spaces on Windows, thus we use a portable solution
	file(WRITE "${CMAKE_SOURCE_DIR}/SourceDir.var" "${CMAKE_SOURCE_DIR}")
	if (NOT EXISTS "${CMAKE_SOURCE_DIR}/SourceDir.var")
		message(FATAL_ERROR "Writing file \"${CMAKE_SOURCE_DIR}/SourceDir.var\" failed!")
	else()
		message(STATUS "Written file \"${CMAKE_SOURCE_DIR}/SourceDir.var\"")
	endif()

	file(WRITE "${CMAKE_SOURCE_DIR}/BuildDir.var" "${CMAKE_BINARY_DIR}")
	if (NOT EXISTS "${CMAKE_SOURCE_DIR}/BuildDir.var")
		message(FATAL_ERROR "Writing file \"${CMAKE_SOURCE_DIR}/BuildDir.var\" failed!")
	else()
		message(STATUS "Written file \"${CMAKE_SOURCE_DIR}/BuildDir.var\"")
	endif()

	# Use our own FFmpeg libraries
	set (BUILTIN_FFMPEG_BUILD_DIR "${CMAKE_BINARY_DIR}/FFmpeg-binaries")
	set (FFMPEG_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/FFmpeg/include" CACHE PATH "FFmpeg headers path")
	ffmpeg_paths("${BUILTIN_FFMPEG_BUILD_DIR}")
	check_ffmpeg_headers(foundHeaders TRUE)
	if (NOT ${foundHeaders})
		message(FATAL_ERROR "Stopping due to previous warnings")
	endif()

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

	set (Decoders_that_you_can_enable aac aac_latm aasc ac3 ac3_fixed adpcm_4xm adpcm_adx adpcm_afc adpcm_ct adpcm_dtk 
	  adpcm_ea adpcm_ea_maxis_xa adpcm_ea_r1 adpcm_ea_r2 adpcm_ea_r3 adpcm_ea_xas adpcm_g722 adpcm_g726
	  adpcm_g726le adpcm_ima_amv adpcm_ima_apc adpcm_ima_dk3 adpcm_ima_dk4 adpcm_ima_ea_eacs adpcm_ima_ea_sead
	  adpcm_ima_iss adpcm_ima_oki adpcm_ima_qt adpcm_ima_rad adpcm_ima_smjpeg adpcm_ima_wav adpcm_ima_ws adpcm_ms
	  adpcm_sbpro_2 adpcm_sbpro_3 adpcm_sbpro_4 adpcm_swf adpcm_thp adpcm_vima adpcm_xa adpcm_yamaha aic alac alias_pix als amrnb
	  amrwb amv anm ansi ape ass asv1 asv2 atrac1 atrac3 atrac3p aura aura2 avrn avrp avs avui ayuv bethsoftvid
	  bfi bink binkaudio_dct binkaudio_rdft bintext bmp bmv_audio bmv_video brender_pix c93 cavs cdgraphics cdxl 
	  cinepak cljr cllc comfortnoise cook cpia cscd cyuv dca dfa dirac dnxhd dpx dsd_lsbf dsd_lsbf_planar dsd_msbf 
	  dsd_msbf_planar dsicinaudio dsicinvideo dvbsub dvdsub dvvideo dxa dxtory eac3 eacmv eamad eatgq eatgv eatqi eightbps 
	  eightsvx_exp eightsvx_fib escape124 escape130 evrc exr ffv1 ffvhuff ffwavesynth fic flac flashsv flashsv2 flic flv 
	  fourxm fraps frwu g2m g723_1 g729 gif gsm gsm_ms h261 h263 h263i h263p h264 h264_crystalhd h264_vda h264_vdpau hevc 
	  hnm4_video huffyuv iac idcin idf iff_byterun1 iff_ilbm imc indeo2 indeo3 indeo4 indeo5 interplay_dpcm interplay_video 
	  jacosub jpeg2000 jpegls jv kgv1 kmvc lagarith libcelt libfdk_aac libgsm libgsm_ms libilbc libopencore_amrnb
	  libopencore_amrwb libopenjpeg libopus libschroedinger libspeex libstagefright_h264 libutvideo libvorbis
	  libvpx_vp8 libvpx_vp9 libzvbi_teletext loco mace3 mace6 mdec metasound microdvd mimic mjpeg mjpegb mlp 
	  mmvideo motionpixels movtext mp1 mp1float mp2 mp2float mp3 mp3adu mp3adufloat mp3float mp3on4 mp3on4float
	  mpc7 mpc8 mpeg1_vdpau mpeg1video mpeg2_crystalhd mpeg2video mpeg4 mpeg4_crystalhd mpeg4_vdpau mpeg_vdpau
	  mpeg_xvmc mpegvideo mpl2 msa1 msmpeg4_crystalhd msmpeg4v1 msmpeg4v2 msmpeg4v3 msrle mss1 mss2 msvideo1 mszh
	  mts2 mvc1 mvc2 mxpeg nellymoser nuv on2avc opus paf_audio paf_video pam pbm pcm_alaw pcm_bluray pcm_dvd pcm_f32be
	  pcm_f32le pcm_f64be pcm_f64le pcm_lxf pcm_mulaw pcm_s16be pcm_s16be_planar pcm_s16le pcm_s16le_planar
	  pcm_s24be pcm_s24daud pcm_s24le pcm_s24le_planar pcm_s32be pcm_s32le pcm_s32le_planar pcm_s8 pcm_s8_planar
	  pcm_u16be pcm_u16le pcm_u24be pcm_u24le pcm_u32be pcm_u32le pcm_u8 pcm_zork pcx pgm pgmyuv pgssub pictor
	  pjs png ppm prores prores_lgpl ptx qcelp qdm2 qdraw qpeg qtrle r10k r210 ra_144 ra_288 ralf rawvideo
	  realtext rl2 roq roq_dpcm rpza rv10 rv20 rv30 rv40 s302m sami sanm sgi sgirle shorten sipr smackaud smacker
	  smc smvjpeg snow sol_dpcm sonic sp5x srt ssa subrip subviewer subviewer1 sunrast svq1 svq3 tak targa
	  targa_y216 text theora thp tiertexseqvideo tiff tmv truehd truemotion1 truemotion2 truespeech tscc tscc2
	  tta twinvq txd ulti utvideo v210 v210x v308 v408 v410 vb vble vc1 vc1_crystalhd vc1_vdpau vc1image vcr1
	  vima vmdaudio vmdvideo vmnc vorbis vp3 vp5 vp6 vp6a vp6f vp7 vp8 vp9 vplayer vqa wavpack webp webvtt
	  wmalossless wmapro wmav1 wmav2 wmavoice wmv1 wmv2 wmv3 wmv3_crystalhd wmv3_vdpau wmv3image wnv1 ws_snd1
	  xan_dpcm xan_wc3 xan_wc4 xbin xbm xface xl xsub xwd y41p yop yuv4 zero12v zerocodec zlib zmbv
	  CACHE STRING "Available decoders" FORCE)
	set (PCM_DECODERS adpcm_4xm adpcm_adx adpcm_afc adpcm_ct adpcm_dtk adpcm_ea adpcm_ea_maxis_xa adpcm_ea_r1
	  adpcm_ea_r2 adpcm_ea_r3 adpcm_ea_xas adpcm_g722 adpcm_g726 adpcm_g726le adpcm_ima_amv adpcm_ima_apc
	  adpcm_ima_dk3 adpcm_ima_dk4 adpcm_ima_ea_eacs adpcm_ima_ea_sead adpcm_ima_iss adpcm_ima_oki adpcm_ima_qt
	  adpcm_ima_rad adpcm_ima_smjpeg adpcm_ima_wav adpcm_ima_ws adpcm_ms adpcm_sbpro_2 adpcm_sbpro_3
	  adpcm_sbpro_4 adpcm_swf adpcm_thp adpcm_vima adpcm_xa adpcm_yamaha interplay_dpcm pcm_alaw pcm_bluray pcm_dvd
	  pcm_f32be pcm_f32le pcm_f64be pcm_f64le pcm_lxf pcm_mulaw pcm_s16be pcm_s16be_planar pcm_s16le
	  pcm_s16le_planar pcm_s24be pcm_s24daud pcm_s24le pcm_s24le_planar pcm_s32be pcm_s32le pcm_s32le_planar
	  pcm_s8 pcm_s8_planar pcm_u16be pcm_u16le pcm_u24be pcm_u24le pcm_u32be pcm_u32le pcm_u8 pcm_zork roq_dpcm
	  sol_dpcm xan_dpcm)
	set (ENABLED_DECODERS theora flac vorbis vp8 vp9 opus ${PCM_DECODERS} CACHE STRING "Enabled audio and video decoders")

	set (CachedSelectedDecoders "")
	set (NeedsFFmpegCompilation TRUE)
	if (EXISTS "${CMAKE_BINARY_DIR}/SelectedDecoders.cache")
		file(READ "${CMAKE_BINARY_DIR}/SelectedDecoders.cache" CachedSelectedDecoders)
		string(STRIP "${CachedSelectedDecoders}" CachedSelectedDecoders)

		if ("${CachedSelectedDecoders}" STREQUAL "${ENABLED_DECODERS}")
			# Check for FFmpeg files
			ffmpeg_paths("${BUILTIN_FFMPEG_BUILD_DIR}")
			check_ffmpeg_headers(foundHeaders FALSE)
			check_ffmpeg_libraries(foundLibraries FALSE)

			if (${foundHeaders} AND ${foundLibraries})
				set(NeedsFFmpegCompilation FALSE)
			endif()
		endif()
	endif()

	message(STATUS "Needs to (re)build FFmpeg: ${NeedsFFmpegCompilation}")

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
	
	if ("${CMAKE_GENERATOR}" MATCHES ".*Makefile.*" OR WINDOWS)
		set(JOBS "1")
	else()
		include(ProcessorCount)
		ProcessorCount(N)
		if(NOT N EQUAL 0)
			set(JOBS ${N})
		else()
			set(JOBS 1)
		endif()
	endif()
	message(STATUS "Parallel jobs: ${JOBS}")

	if (${NeedsFFmpegCompilation})
		add_shell_command(FFmpeg
			OUTPUT ${FFMPEG_LIBRARIES}
			DEPENDS "${CMAKE_BINARY_DIR}/CMakeCache.txt"
			COMMAND ./build_ffmpeg.sh ${OS_ARG} ${VS_ARG} ${JOBS} ${ENABLED_DECODERS})
	endif()

	# ============== Install rules =============
	if (NOT MACOSX)
		if (WINDOWS)
			install(DIRECTORY "${BUILTIN_FFMPEG_BUILD_DIR}/bin"
					DESTINATION .
					COMPONENT binaries
					FILES_MATCHING PATTERN "*-*.dll")
		else()
			install(DIRECTORY "${BUILTIN_FFMPEG_BUILD_DIR}/bin"
					DESTINATION .
					COMPONENT binaries)
		endif()
	endif()

	if (LINUX)
		install(DIRECTORY "${BUILTIN_FFMPEG_BUILD_DIR}/lib"
				DESTINATION .
				COMPONENT binaries)
	endif()
	# For OS X, the FFmpeg libs are bundled in the sfeMovie framework
endif()

message(STATUS "Using these FFmpeg libraries:")
foreach(library ${FFMPEG_LIBRARIES})
	message(STATUS "- ${library}")
endforeach()

