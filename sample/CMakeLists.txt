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

target_link_libraries(
    ${SFEMOVIE_SAMPLE}
    ${SFEMOVIE_LIB}
    ${SFML_LIBRARIES}
    ${SFML_DEPENDENCIES}
)

if (MACOSX)
    set_target_properties(${SFEMOVIE_SAMPLE} PROPERTIES 
                          BUILD_WITH_INSTALL_RPATH 1 
                          INSTALL_RPATH "@executable_path/")
endif()
