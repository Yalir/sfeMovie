
add_subdirectory(gtest)
set_target_properties(gtest PROPERTIES FOLDER "Tests")
set_target_properties(gtest_main PROPERTIES FOLDER "Tests")
enable_testing()

add_custom_target(AllTests COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --build-config "$<CONFIGURATION>")
set_target_properties(AllTests PROPERTIES FOLDER "Tests")

macro(add_full_test testname)
    add_executable(${testname} ${testname}.cpp)
    target_link_libraries(${testname} ${SFML_SYSTEM_LIBRARY} sfeMovie gtest gtest_main)

    # Copy libraries next to the test
    add_custom_command(TARGET ${testname} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}" $<TARGET_FILE_DIR:${testname}>)

    if(MACOSX)
        set_target_properties(${testname} PROPERTIES 
            BUILD_WITH_INSTALL_RPATH 1 
            INSTALL_RPATH "@executable_path/")
    endif ()
    
    set_target_properties(${testname} PROPERTIES FOLDER "Tests")
    source_group("" FILES ${testname}.cpp)
    add_dependencies(AllTests ${testname})
endmacro()

# sfeMovie tests
add_full_test(TimerTest)
add_full_test(DemuxerTest)
configure_file("small_1.ogv" "small_1.ogv" COPYONLY)
configure_file("long_1.wav" "long_1.wav" COPYONLY)
configure_file("left-right.wav" "left-right.wav" COPYONLY)

configure_file("voice_1.mp3" "voice_1.mp3" COPYONLY)
configure_file("small_2.mp3" "small_2.mp3" COPYONLY)
configure_file("small_3.flac" "small_3.flac" COPYONLY)
configure_file("small_4.wav" "small_4.wav" COPYONLY)
