
# define the install directory for miscellaneous files
if(WINDOWS)
    set(INSTALL_MISC_DIR . )
else()
    set(INSTALL_MISC_DIR "share/${SFEMOVIE_LIB}")
endif()

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

if (WINDOWS)
    install(FILES
            "${CMAKE_SOURCE_DIR}/deps/Windows-i386/libgcc_s_dw2-1.dll"
            "${CMAKE_SOURCE_DIR}/deps/Windows-i386/libiconv-2.dll"
            DESTINATION bin
            COMPONENT binaries)
endif()

install(FILES "${CMAKE_SOURCE_DIR}/License.txt" "${CMAKE_SOURCE_DIR}/ReadMe.txt" "${CMAKE_SOURCE_DIR}/Authors.txt"
        DESTINATION ${INSTALL_MISC_DIR}
        COMPONENT resources)
