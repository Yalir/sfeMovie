
set(SFEMOVIE_MINIMALIST_DEMO_SRC
    main.cpp
)
source_group("" FILES ${SFEMOVIE_MINIMALIST_DEMO_SRC})

add_executable(sfeMovieMinimalistDemo ${SFEMOVIE_MINIMALIST_DEMO_SRC})
set_target_properties(sfeMovieMinimalistDemo PROPERTIES FOLDER "Examples")

if (SFEMOVIE_BUILD_STATIC)
    set (SFML_STATIC_LIBRARIES TRUE)
    find_package(SFML 2 COMPONENTS graphics window system audio REQUIRED)

    set_target_properties(sfeMovieMinimalistDemo PROPERTIES
        COMPILE_DEFINITIONS SFML_STATIC
        COMPILE_DEFINITIONS SFEMOVIE_STATIC)
    target_link_libraries(sfeMovieMinimalistDemo PRIVATE
        sfeMovie
        ${SFML_LIBRARIES}
        ${SFML_DEPENDENCIES})
else()
    target_link_libraries(sfeMovieMinimalistDemo PRIVATE
        sfeMovie
        ${SFML_LIBRARIES}
        ${SFML_DEPENDENCIES}
    )
endif()

if (MACOSX)
    set_target_properties(sfeMovieMinimalistDemo PROPERTIES 
        BUILD_WITH_INSTALL_RPATH 1 
        INSTALL_RPATH "@executable_path/")
endif()
