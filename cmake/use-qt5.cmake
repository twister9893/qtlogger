find_package(Qt5 COMPONENTS Core Gui Widgets Network Sql Svg Xml REQUIRED)

add_definitions(${Qt5Core_DEFINITIONS} 
                ${Qt5Gui_DEFINITIONS} 
                ${Qt5Widgets_DEFINITIONS} 
                ${Qt5Network_DEFINITIONS} 
                ${Qt5Xml_DEFINITIONS}
                ${Qt5Sql_DEFINITIONS}
                ${Qt5Svg_DEFINITIONS})

include_directories(${Qt5Core_INCLUDES} 
                    ${Qt5Gui_INCLUDES} 
                    ${Qt5Widgets_INCLUDES} 
                    ${Qt5Network_INCLUDES} 
                    ${Qt5Xml_INCLUDES}
                    ${Qt5Sql_INCLUDES}
                    ${Qt5Svg_INCLUDES})

include(qt-find-moc-headers)
