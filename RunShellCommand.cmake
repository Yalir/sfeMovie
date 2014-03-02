include(CMakeParseArguments)

function(add_shell_command customTargetName)
	cmake_parse_arguments(THIS "" "" "OUTPUT;DEPENDS;COMMAND" ${ARGN})

	if (NOT DEFINED THIS_OUTPUT OR NOT DEFINED THIS_COMMAND OR NOT DEFINED THIS_DEPENDS)
		message(FATAL_ERROR "Invalid arguments given to add_shell_command(newTargetName DEPENDS generatedDependencies COMMAND shell_command")
	endif()


	if (WINDOWS)
		add_custom_target(${customTargetName} ALL DEPENDS ${THIS_OUTPUT}) 
		add_custom_command(OUTPUT ${THIS_OUTPUT}
					COMMAND BatchBridgeToShell ARGS ${MINGW_DIR} ${THIS_COMMAND}
					DEPENDS "${THIS_DEPENDS}"
					WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
	else()
		add_custom_target(${customTargetName} ALL DEPENDS ${THIS_OUTPUT}) 
		add_custom_command(OUTPUT ${THIS_OUTPUT}
					COMMAND bash ARGS -c \"${THIS_COMMAND}\"
					DEPENDS "${THIS_DEPENDS}"
					WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
	endif()
endfunction(add_shell_command)
