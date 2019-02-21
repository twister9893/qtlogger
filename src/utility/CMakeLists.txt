set(TARGET qtl)
qt_add_executable(${TARGET})

add_definitions(-DCONFIG_PATH="${PROJECT_SOURCE_DIR}/data")
