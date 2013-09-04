
function(RunShell target phase shell_command)
	set(cmd ${ARGV})
	list(REMOVE_AT cmd 0 1)
	
	if (MSVC)
		add_custom_target(FFmpeg ALL DEPENDS ${FFMPEG_LIBRARIES}) 
		add_custom_command(OUTPUT ${FFMPEG_LIBRARIES}
					COMMAND BatchBridgeToShell ARGS ${MINGW_DIR} ${cmd}
					DEPENDS "${CMAKE_BINARY_DIR}/CMakeCache.txt"
					WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
	else()
		add_custom_target(FFmpeg ALL DEPENDS ${FFMPEG_LIBRARIES}) 
		add_custom_command(OUTPUT ${FFMPEG_LIBRARIES}
					COMMAND bash ARGS -c \"${cmd}\"
					DEPENDS "${CMAKE_BINARY_DIR}/CMakeCache.txt"
					WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
	endif()
endfunction(RunShell)
