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
