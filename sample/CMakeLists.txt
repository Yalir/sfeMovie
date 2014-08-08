set(SFEMOVIE_SAMPLE "sfeMovieSample")

add_executable(
    ${SFEMOVIE_SAMPLE}
    main.cpp
)

if (SFEMOVIE_BUILD_STATIC)
	set (SFML_STATIC_LIBRARIES TRUE)
	find_package (SFML 2 COMPONENTS graphics window system audio REQUIRED)
	message(STATUS "SFML libraries: ${SFML_LIBRARIES}")
	message(STATUS "Additional dependencies: ${SFML_DEPENDENCIES}")

	set_target_properties(${SFEMOVIE_SAMPLE} PROPERTIES
		COMPILE_DEFINITIONS SFML_STATIC
		COMPILE_DEFINITIONS SFEMOVIE_STATIC)
	target_link_libraries(
		${SFEMOVIE_SAMPLE}
		${SFEMOVIE_LIB}
		${FFMPEG_LIBRARIES}
		${OTHER_LIBRARIES}
		${SFML_LIBRARIES}
		${SFML_DEPENDENCIES})
else()
	target_link_libraries(
	    ${SFEMOVIE_SAMPLE}
	    ${SFEMOVIE_LIB}
	    ${SFML_LIBRARIES}
	    ${SFML_DEPENDENCIES}
	)
endif()

if (MACOSX)
	set_target_properties(${SFEMOVIE_SAMPLE} PROPERTIES 
	                      BUILD_WITH_INSTALL_RPATH 1 
	                      INSTALL_RPATH "@executable_path/")
endif()