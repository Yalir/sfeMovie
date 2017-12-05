include(CMakeParseArguments)

function(add_shell_command customTargetName)
	cmake_parse_arguments(THIS "" "" "OUTPUT;DEPENDS;COMMAND" ${ARGN})

	if (NOT DEFINED THIS_OUTPUT OR NOT DEFINED THIS_COMMAND OR NOT DEFINED THIS_DEPENDS)
		message(FATAL_ERROR "Invalid arguments given to add_shell_command(newTargetName OUTPUT output DEPENDS generatedDependencies COMMAND shell_command")
	endif()


	if (MSVC)
		string(REPLACE ";" " " SPACED_COMMAND "${THIS_COMMAND}")

		add_custom_target(${customTargetName} ALL DEPENDS ${THIS_OUTPUT}) 
		add_custom_command(OUTPUT ${THIS_OUTPUT}
					COMMAND ${CMAKE_COMMAND} -E env MSYS2_PATH_TYPE=inherit ${FFMPEG_BASH_EXE} -c -l "cd ${CMAKE_SOURCE_DIR} && ${SPACED_COMMAND}"
					DEPENDS "${THIS_DEPENDS}"
					VERBATIM)
	else()
		add_custom_target(${customTargetName} ALL DEPENDS ${THIS_OUTPUT}) 
		add_custom_command(OUTPUT ${THIS_OUTPUT}
					COMMAND bash ARGS -c \"${THIS_COMMAND}\"
					DEPENDS "${THIS_DEPENDS}"
					WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
	endif()
endfunction(add_shell_command)
