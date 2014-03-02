
set_target_properties(${SFEMOVIE_LIB} PROPERTIES
					  DEBUG_POSTFIX "-d"
					  LINK_FLAGS "${LINKER_FLAGS}"
					  SOVERSION "${VERSION_MAJOR}.${VERSION_MINOR}.0"
                      VERSION "${VERSION_MAJOR}.${VERSION_MINOR}")


if (MACOSX)
    # edit target properties
    set_target_properties(${SFEMOVIE_LIB} PROPERTIES 
                          FRAMEWORK TRUE
                          FRAMEWORK_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}
                          MACOSX_FRAMEWORK_IDENTIFIER org.yalir.${SFEMOVIE_LIB}
                          MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}
                          MACOSX_FRAMEWORK_BUNDLE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}
                          PUBLIC_HEADER ${SFE_HEADERS} )

    # adapt install directory to allow distributing dylibs/frameworks in user’s frameworks/application bundle
    set_target_properties(${SFEMOVIE_LIB} PROPERTIES 
                          BUILD_WITH_INSTALL_RPATH 1 
                          INSTALL_NAME_DIR "@rpath")
endif()
