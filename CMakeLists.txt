cmake_minimum_required(VERSION 3.21)
project(QtLanWebSocket VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5.3 REQUIRED COMPONENTS Quick WebSockets Network)

qt_standard_project_setup()

qt_add_executable(appQtLanWebSocket
    main.cpp
    WebSocketBackend.h
    WebSocketBackend.cpp
)

qt_add_qml_module(appQtLanWebSocket
    URI QtLanWebSocket
    VERSION 1.0
    QML_FILES Main.qml
)

target_link_libraries(appQtLanWebSocket PRIVATE
    Qt6::Quick
    Qt6::WebSockets
    Qt6::Network
)

if(WIN32)
    add_custom_command(TARGET appQtLanWebSocket POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/web"
        "$<TARGET_FILE_DIR:appQtLanWebSocket>/web"
    )
endif()
