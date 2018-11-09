function(qt_add_library target)
    set(LIB_DEPS)

    foreach(LIB ${ARGN})
        if(NOT ${LIB} STREQUAL ${target})
            set(LIB_DEPS ${LIB_DEPS} ${LIB})
        endif()
    endforeach()

    include_directories(${CMAKE_CURRENT_BINARY_DIR}
                        ${CMAKE_CURRENT_SOURCE_DIR})

    file(GLOB_RECURSE HEADER_FILES *.h)
    file(GLOB_RECURSE SOURCE_FILES *.cpp)
    file(GLOB_RECURSE UI_FILES *.ui)
    file(GLOB_RECURSE RESOURCE_FILES *.rcs
                                     *.qrc)

    qt_find_moc_headers(MOC_HEADER_FILES ${HEADER_FILES})

    qt5_wrap_cpp(${target}_MOCrcs ${MOC_HEADER_FILES})
    qt5_wrap_ui(${target}_UIrcs ${UI_FILES})
    qt5_add_resources(${target}_RESrcs ${RESOURCE_FILES})

    add_library(${target} SHARED ${SOURCE_FILES}
                                 ${${target}_MOCrcs}
                                 ${${target}_UIrcs}
                                 ${${target}_RESrcs})

    target_link_libraries(${target} ${QT_LIBRARIES} ${LIB_DEPS})

endfunction()
