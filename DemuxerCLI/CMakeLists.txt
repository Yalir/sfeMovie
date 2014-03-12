set(DEMUXER_CLI "demux")

add_executable(
    ${DEMUXER_CLI}
    main.cpp
)

target_link_libraries(
    ${DEMUXER_CLI}
    ${SFEMOVIE_LIB}
    ${SFML_SYSTEM_LIBRARY}
)

if (MACOSX)
	set_target_properties(${DEMUXER_CLI} PROPERTIES 
	                      BUILD_WITH_INSTALL_RPATH 1 
	                      INSTALL_RPATH "@executable_path/")
endif()
