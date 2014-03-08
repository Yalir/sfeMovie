set(SFEMOVIE_SAMPLE "sfeMovieSample")

add_executable(
    ${SFEMOVIE_SAMPLE}
    main.cpp
)

target_link_libraries(
    ${SFEMOVIE_SAMPLE}
    ${SFEMOVIE_LIB}
    ${SFML_LIBRARIES}
)

if (MACOSX)
	set_target_properties(${SFEMOVIE_SAMPLE} PROPERTIES 
	                      BUILD_WITH_INSTALL_RPATH 1 
	                      INSTALL_RPATH "@executable_path/")
endif()