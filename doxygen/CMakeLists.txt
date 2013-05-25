
if(BUILD_DOC)
	find_package(Doxygen REQUIRED)

	# define the install directory for miscellaneous files
	if(WINDOWS)
		set(INSTALL_MISC_DIR .)
	else()
		set(INSTALL_MISC_DIR share/sfeMovie)
	endif()

	# set the input and output documentation paths
	set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR})
	set(DOXYGEN_OUTPUT_DIR ${PROJECT_BINARY_DIR}/doxygen)


	# configure the source Doxyfile by copying it and replacing all @variables@
	set(DOXYGEN_CONFIGURED_INPUT ${DOXYGEN_OUTPUT_DIR}/Doxyfile)
	configure_file(${DOXYGEN_INPUT_DIR}/doxygen/Doxyfile ${DOXYGEN_CONFIGURED_INPUT} @ONLY)

	# copy the files needed by the documentation
	configure_file(${DOXYGEN_INPUT_DIR}/doxygen/doxygen.css ${DOXYGEN_OUTPUT_DIR}/doxygen.css COPYONLY)
	configure_file(${DOXYGEN_INPUT_DIR}/doxygen/footer.html ${DOXYGEN_OUTPUT_DIR}/footer.html COPYONLY)
	configure_file(${DOXYGEN_INPUT_DIR}/doxygen/header.html ${DOXYGEN_OUTPUT_DIR}/header.html COPYONLY)

	# target setup
	add_custom_target(doxygen ALL
		              COMMAND ${CMAKE_COMMAND} -E echo_append "Building Documentation..."
		              COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIGURED_INPUT}
		              COMMAND ${CMAKE_COMMAND} -E echo "Done."
		              WORKING_DIRECTORY ${DOXYGEN_INPUT_DIR})

	# setup install rules
	install(DIRECTORY ${DOXYGEN_OUTPUT_DIR}/html
		    DESTINATION ${INSTALL_MISC_DIR}/doxygen
		    COMPONENT doxygen)
endif()
