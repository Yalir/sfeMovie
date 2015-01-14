set(SFEMOVIE_SAMPLE "sfeMovieSample")

set(SFEMOVIE_SAMPLE_SRC
    main.cpp
    MediaInfo.cpp
    MediaInfo.hpp
    StreamSelector.cpp
    StreamSelector.hpp
    UserInterface.cpp
    UserInterface.hpp
)

add_executable(
    ${SFEMOVIE_SAMPLE}
    ${SFEMOVIE_SAMPLE_SRC}
)
source_group("Sources" FILES ${SFEMOVIE_SAMPLE_SRC})

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
		${LIBASS_LIBRARIES}
		${FFMPEG_LIBRARIES}
		${OTHER_LIBRARIES}
		${SFML_LIBRARIES}
		${SFML_DEPENDENCIES})
else()
	target_link_libraries(
	    ${SFEMOVIE_SAMPLE}
	    ${SFEMOVIE_LIB}
	    ${LIBASS_LIBRARIES}
	    ${SFML_LIBRARIES}
	    ${SFML_DEPENDENCIES}
	)
endif()

if (MACOSX)
    set_target_properties(${SFEMOVIE_SAMPLE} PROPERTIES 
                          BUILD_WITH_INSTALL_RPATH 1 
                          INSTALL_RPATH "@executable_path/")
endif()
