
function(RunShell target phase shell_command)
	set(cmd ${ARGV})
	list(REMOVE_AT cmd 0 1)
	
	if (MSVC)
		add_custom_target(BuildFFMPEG ALL DEPENDS "ffmpeg-libs") 
		add_custom_command(OUTPUT "ffmpeg-libs"
					COMMAND BatchBridgeToShell ARGS ${MINGW_DIR} ${cmd}
					WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
	else()
		add_custom_target(BuildFFMPEG ALL DEPENDS "ffmpeg-libs") 
		add_custom_command(OUTPUT "ffmpeg-libs"
					COMMAND bash ARGS -c \"${cmd}\"
					WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
	endif()

	# Linux and MSVC use FFmpeg's dynamic library (instead of static FFmpeg libraries)
	if (MSVC OR LINUX)
		file(GLOB FFMPEG_BUILT_LIBS RELATIVE ${PROJECT_SOURCE_DIR} "${BUILTIN_FFMPEG_BUILD_DIR}/*")

		if (MSVC)
			set (FFMPEG_INSTALL_DIR "bin")
		else ()
			set (FFMPEG_INSTALL_DIR "lib")
		endif()

	    install(FILES ${FFMPEG_BUILT_LIBS}
	            DESTINATION ${CMAKE_INSTALL_PREFIX}/${FFMPEG_INSTALL_DIR}
	            COMPONENT devel)
	endif()
endfunction(RunShell)
