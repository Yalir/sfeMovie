
if (MSVC)
  target_compile_options(${SFEMOVIE_LIB} PRIVATE "$<$<CONFIG:Release>:/O2>")
  target_compile_options(${SFEMOVIE_LIB} PRIVATE "$<$<CONFIG:Debug>:/Od>")
else()
  target_compile_options(${SFEMOVIE_LIB} PRIVATE "$<$<CONFIG:Release>:-O2>")
  target_compile_options(${SFEMOVIE_LIB} PRIVATE "$<$<CONFIG:Debug>:-O0>")
endif()

if (SFEMOVIE_BUILD_STATIC)
  set(STATIC_POSTFIX "-s")
else()
  set(STATIC_POSTFIX "")
endif()

set_target_properties(${SFEMOVIE_LIB} PROPERTIES
                      DEBUG_POSTFIX "${STATIC_POSTFIX}-d"
                      RELEASE_POSTFIX "${STATIC_POSTFIX}"
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
                          PUBLIC_HEADER "${HEADER_FILES}")

    # adapt install directory to allow distributing dylibs/frameworks in user’s frameworks/application bundle
    set_target_properties(${SFEMOVIE_LIB} PROPERTIES 
                          BUILD_WITH_INSTALL_RPATH 1 
                          INSTALL_NAME_DIR "@rpath")
endif()
