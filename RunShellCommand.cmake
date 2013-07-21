
function(RunShell target phase shell_command)
	set(cmd ${ARGV})
	list(REMOVE_AT cmd 0 1)
	
	if (MSVC)
		add_custom_command(TARGET ${target}
                     ${phase}
                     COMMAND BatchBridgeToShell ARGS ${MINGW_DIR} ${cmd}
					 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})	
	else()
		message(STATUS "add_custom_command(TARGET ${target} ${phase} COMMAND bash ARGS -c \"${cmd}\" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}")
		add_custom_target(BuildFFMPEG ALL DEPENDS "ffmpeg-libs") 
		add_custom_command(OUTPUT "ffmpeg-libs"
					COMMAND bash ARGS -c \"${cmd}\"
					WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
		# add_custom_command(TARGET ${target}
  #                    ${phase}
  #                    COMMAND bash ARGS -c \"${cmd}\"
		# 			 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})	
	endif()
endfunction(RunShell)
