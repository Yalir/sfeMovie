
if (CMAKE_CONFIGURATION_TYPES)
    add_custom_target(VERBOSE_RUN_TESTS COMMAND ${CMAKE_CTEST_COMMAND}
        --force-new-ctest-process --verbose
        --build-config "$<CONFIGURATION>")
else()
    add_custom_target(VERBOSE_RUN_TESTS COMMAND ${CMAKE_CTEST_COMMAND}
        --force-new-ctest-process --verbose)
endif()

macro(add_full_test testname)
	link_directories(${EXTERNAL_LIBRARIES_DIR})
	add_executable(${testname} ${testname}.cpp)
	if (WINDOWS)
		target_link_libraries(${testname} sfeMovie)
		add_dependencies(${testname} sfeMovie)

		# Copy DLLs next to the test executables
		add_custom_command(TARGET ${testname} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}"
			$<TARGET_FILE_DIR:${testname}>)
	elseif(MACOSX)
		target_link_libraries(${testname} sfeMovie boost_unit_test_framework)
		set_target_properties(${testname} PROPERTIES INSTALL_RPATH "@rpath")

		# Copy dylibs next to the test executables
		add_custom_command(TARGET ${testname} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}"
			$<TARGET_FILE_DIR:${testname}>)
	else ()
		target_link_libraries(${testname} sfeMovie boost_unit_test_framework)
	endif ()
	add_test(NAME ${testname} COMMAND ${testname})
endmacro()

add_full_test(TimerTest)
add_full_test(DemuxerTest)
configure_file("small_1.ogv" "small_1.ogv" COPYONLY)
configure_file("long_1.wav" "long_1.wav" COPYONLY)
configure_file("left-right.wav" "left-right.wav" COPYONLY)

configure_file("voice_1.mp3" "voice_1.mp3" COPYONLY)
configure_file("small_2.mp3" "small_2.mp3" COPYONLY)
configure_file("small_3.flac" "small_3.flac" COPYONLY)
