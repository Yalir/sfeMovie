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

message(STATUS "target_link_libraries(${DEMUXER_CLI} ${SFEMOVIE_LIB})")
