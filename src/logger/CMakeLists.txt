set(TARGET qtlogger)
qt_add_library(${TARGET})

add_definitions(-DCONFIG_PATH="${PROJECT_SOURCE_DIR}/data")
