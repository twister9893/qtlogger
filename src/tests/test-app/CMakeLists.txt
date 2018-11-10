set(TARGET test-app)
qt_add_executable(${TARGET} qtlogger)

add_definitions(-DQT_MESSAGELOGCONTEXT)
