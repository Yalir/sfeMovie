
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
			${FFMPEG_LIBSWRESAMPLE_LIBRARIES} CACHE STRING "FFmpeg libraries paths")
endmacro(ffmpeg_paths)

# Check that all of the FFmpeg headers can be found in ${FFMPEG_INCLUDE_DIR}
macro(check_ffmpeg_headers)
	foreach(header "libavcodec/avcodec.h" "libavdevice/avdevice.h" "libavformat/avformat.h" "libavutil/avutil.h" "libswscale/swscale.h" "libswresample/swresample.h")
		if(NOT EXISTS "${FFMPEG_INCLUDE_DIR}/${header}")
			message(FATAL_ERROR "The chosen FFmpeg is missing a header file: ${FFMPEG_INCLUDE_DIR}/${header}")
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
