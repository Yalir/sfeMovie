set(SFEMOVIE_SAMPLE "sfeMovieSample")

add_executable(
    ${SFEMOVIE_SAMPLE}
    main.cpp
)

target_link_libraries(
    ${SFEMOVIE_SAMPLE}
    ${LIB_NAME}
    ${SFML_LIBRARIES}
    ${FFMPEG_LIBRARIES}
)
