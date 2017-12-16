
set(SFE_TEST_DATA_DIR "${CMAKE_BINARY_DIR}/TestData" CACHE INTERNAL "Path to the test data")

add_subdirectory(gtest)
set_target_properties(gtest PROPERTIES FOLDER "Tests")
set_target_properties(gtest_main PROPERTIES FOLDER "Tests")
enable_testing()

add_custom_target(AllTests COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --build-config "$<CONFIGURATION>")
set_target_properties(AllTests PROPERTIES FOLDER "Tests")

# Usage sfe_add_test(TARGET target_name SOURCES src1.cpp src2.cpp TEST_FILES f1 f2 ENVIRONMENT "param=value")
function(sfe_add_test)
    cmake_parse_arguments(THIS "" "TARGET" "SOURCES;TEST_FILES;ENVIRONMENT" ${ARGN})
    if (THIS_UNPARSED_ARGUMENTS)
        message(STATUS "Unparsed arguments: ${THIS_UNPARSED_ARGUMENTS}")
        message(FATAL_ERROR "Usage: sfe_add_test(TARGET target_name SOURCES src1.cpp src2.cpp [TEST_FILES f1 f2] [ENVIRONMENT \"param=value\"])")
    endif()
    if (NOT "${THIS_TARGET}" MATCHES "UT.*")
        message(FATAL_ERROR "Test name must start with UT")
    endif()

    file(COPY ${THIS_TEST_FILES} DESTINATION ${SFE_TEST_DATA_DIR})

    add_executable(${THIS_TARGET} ${THIS_SOURCES})
    add_test(NAME ${THIS_TARGET} COMMAND $<TARGET_FILE:${THIS_TARGET}>)
    target_link_libraries(${THIS_TARGET} PRIVATE sfeMovie sfeMovie_PrivateHeaders gtest gtest_main ${SFML_SYSTEM_LIBRARY})
    target_compile_definitions(${THIS_TARGET} PRIVATE "TEST_DATA_DIR=\"${SFE_TEST_DATA_DIR}/\"")
    set_target_properties(${THIS_TARGET} PROPERTIES FOLDER "Tests")
    source_group("" FILES ${THIS_SOURCES})

    # Copy libraries next to the test
    add_custom_command(TARGET ${THIS_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:sfeMovie>" "$<TARGET_FILE_DIR:${THIS_TARGET}>")

    if(APPLE)
        set_target_properties(${THIS_TARGET} PROPERTIES 
            BUILD_WITH_INSTALL_RPATH 1 
            INSTALL_RPATH "@executable_path/")
    endif ()
    
    add_dependencies(AllTests ${THIS_TARGET})
endfunction()

# sfeMovie tests
sfe_add_test(TARGET UTTimer SOURCES TimerTest.cpp)
sfe_add_test(TARGET UTDemuxer SOURCES DemuxerTest.cpp
    TEST_FILES small_1.ogv long_1.wav left-right.wav voice_1.mp3 small_2.mp3 small_3.flac small_4.wav)
