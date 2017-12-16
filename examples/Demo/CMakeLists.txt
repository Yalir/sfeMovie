
set(SFEMOVIE_DEMO_SRC
    main.cpp
    MediaInfo.cpp
    MediaInfo.hpp
    StreamSelector.cpp
    StreamSelector.hpp
    UserInterface.cpp
    UserInterface.hpp
)
source_group("" FILES ${SFEMOVIE_DEMO_SRC})

add_executable(sfeMovieDemo ${SFEMOVIE_DEMO_SRC})
set_target_properties(sfeMovieDemo PROPERTIES FOLDER "Examples")

if (SFEMOVIE_BUILD_STATIC)
    set (SFML_STATIC_LIBRARIES TRUE)
    find_package (SFML 2 COMPONENTS graphics window system audio REQUIRED)

    set_target_properties(sfeMovieDemo PROPERTIES
        COMPILE_DEFINITIONS SFML_STATIC
        COMPILE_DEFINITIONS SFEMOVIE_STATIC)
    target_link_libraries(sfeMovieDemo PRIVATE
        sfeMovie
        ${SFML_LIBRARIES}
        ${SFML_DEPENDENCIES})
else()
    target_link_libraries(sfeMovieDemo PRIVATE
        sfeMovie
        ${SFML_LIBRARIES}
        ${SFML_DEPENDENCIES}
    )
endif()

if (MACOSX)
    set_target_properties(sfeMovieDemo PROPERTIES 
        BUILD_WITH_INSTALL_RPATH 1 
        INSTALL_RPATH "@executable_path/")
endif()
