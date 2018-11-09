function(qt_find_moc_headers moc_headers_var)

    foreach(FILE ${ARGN})
        file(READ ${FILE} CONTENT)
        string(REGEX MATCH "Q_OBJECT" RES "${CONTENT}")
        if(NOT ${RES} EQUAL -1)
            set(${moc_headers_var} ${${moc_headers_var}} ${FILE})
        endif()
    endforeach()
    set(${moc_headers_var} ${${moc_headers_var}} ${FILE} PARENT_SCOPE)

endfunction()
