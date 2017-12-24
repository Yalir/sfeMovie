
# Usage delayed_copy(TARGET target_name DESTINATION_DIR dir FILES f1 f2)
function(delayed_copy)
    cmake_parse_arguments(THIS "" "TARGET;DESTINATION_DIR" "FILES" ${ARGN})
    if (THIS_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unparsed arguments: ${THIS_UNPARSED_ARGUMENTS}")
    endif()

    if (NOT THIS_TARGET)
        message(FATAL_ERROR "Missing TARGET argument")
    endif()
    if (NOT THIS_DESTINATION_DIR)
        message(FATAL_ERROR "Missing DESTINATION_DIR argument")
    endif()
    if (NOT THIS_FILES)
        message(FATAL_ERROR "Missing FILES argument")
    endif()
    

    if (NOT CMAKE_HOST_WIN32)
        foreach(FILE IN LISTS THIS_FILES)
            get_filename_component(FILENAME "${FILE}" NAME)
            add_custom_command(TARGET ${THIS_TARGET} POST_BUILD
                COMMAND rm -rf "${THIS_DESTINATION_DIR}/${FILENAME}")
            add_custom_command(TARGET ${THIS_TARGET} POST_BUILD
                COMMAND cp -Rpf "${FILE}" "${THIS_DESTINATION_DIR}/")
        endforeach()
    endif()
endfunction()
