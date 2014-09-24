
# Install sfeMovie library and headers
if (LINUX OR WINDOWS)
	install(DIRECTORY include
			DESTINATION .)

	install(TARGETS ${SFEMOVIE_LIB}
        RUNTIME DESTINATION bin COMPONENT binaries
        LIBRARY DESTINATION lib${LIB_SUFFIX} COMPONENT binaries
        ARCHIVE DESTINATION lib${LIB_SUFFIX} COMPONENT libraries)
else()
	install(TARGETS ${SFEMOVIE_LIB}
            FRAMEWORK
            DESTINATION ${CMAKE_INSTALL_FRAMEWORK_PREFIX}
            COMPONENT binaries)
endif()
