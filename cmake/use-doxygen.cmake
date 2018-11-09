find_package(Doxygen)

if(DOXYGEN_FOUND)
    option(DOXYGEN_WARN_UNDOC "Generate Doxygen warnings for undocumented members?")
    if(DOXYGEN_WARN_UNDOC)
        set(CMAKE_WARN_IF_UNDOCUMENTED "YES")
    else()
        set(CMAKE_WARN_IF_UNDOCUMENTED "NO")
    endif()

    set(DOXYGEN_INPUT ${PROJECT_SOURCE_DIR}/src/core)
    set(DOXYGEN_OUTPUT ${PROJECT_SOURCE_DIR}/doc)
    set(DOXYGEN_HTML_INDEX ${PROJECT_SOURCE_DIR}/doc/html/index.html)

    configure_file(${CMAKE_SOURCE_DIR}/Doxyfile.in
                   ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    )
    add_custom_command(OUTPUT ${DOXYGEN_HTML_INDEX}
                       COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
                       DEPENDS interface
    )
    add_custom_target(doc ALL
                      DEPENDS ${DOXYGEN_HTML_INDEX}
    )
endif()
