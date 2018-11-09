find_package(Qt4)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(QT_USE_QTXML 1)
set(QT_USE_QTNETWORK 1)
set(QT_USE_QTDESIGNER 1)

include(${QT_USE_FILE})
include(qt-find-moc-headers)
