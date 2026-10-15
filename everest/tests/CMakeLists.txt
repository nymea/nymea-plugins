cmake_minimum_required(VERSION 3.16)
project(everest-rfid-tests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
enable_testing()
set(NYMEA_TEST_QT_MAJOR_VERSION 6 CACHE STRING "Qt major version used by the standalone tests")
find_package(Qt${NYMEA_TEST_QT_MAJOR_VERSION} REQUIRED COMPONENTS Core DBus Test Network WebSockets)
find_program(DBUS_RUN_SESSION dbus-run-session REQUIRED)
set(NYMEA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../nymea" CACHE PATH "nymea source checkout (headers only)")
add_executable(tst_rfidproviderclient tst_rfidproviderclient.cpp ../rfid/rfidproviderclient.cpp)
target_include_directories(tst_rfidproviderclient PRIVATE .. "${NYMEA_SOURCE_DIR}/libnymea")
target_link_libraries(tst_rfidproviderclient PRIVATE
    Qt${NYMEA_TEST_QT_MAJOR_VERSION}::Core Qt${NYMEA_TEST_QT_MAJOR_VERSION}::DBus
    Qt${NYMEA_TEST_QT_MAJOR_VERSION}::Test Qt${NYMEA_TEST_QT_MAJOR_VERSION}::Network
    Qt${NYMEA_TEST_QT_MAJOR_VERSION}::WebSockets)
add_test(NAME everest-rfid COMMAND ${DBUS_RUN_SESSION} -- $<TARGET_FILE:tst_rfidproviderclient>)

find_package(PkgConfig REQUIRED)
pkg_check_modules(NYMEA REQUIRED IMPORTED_TARGET nymea)
find_program(NYMEA_PLUGININFO_COMPILER nymea-plugininfocompiler REQUIRED)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/plugininfo.h ${CMAKE_CURRENT_BINARY_DIR}/extern-plugininfo.h
    COMMAND ${NYMEA_PLUGININFO_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR}/../integrationplugineverest.json
            -o ${CMAKE_CURRENT_BINARY_DIR}/plugininfo.h -e ${CMAKE_CURRENT_BINARY_DIR}/extern-plugininfo.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../integrationplugineverest.json)
add_executable(tst_everestconnection tst_everestconnection.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/plugininfo.h
    ../jsonrpc/everestconnection.cpp ../jsonrpc/everestevse.cpp ../jsonrpc/everestjsonrpcclient.cpp
    ../jsonrpc/everestjsonrpcinterface.cpp ../jsonrpc/everestjsonrpcreply.cpp)
target_include_directories(tst_everestconnection PRIVATE .. ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(tst_everestconnection PRIVATE PkgConfig::NYMEA
    Qt${NYMEA_TEST_QT_MAJOR_VERSION}::Core Qt${NYMEA_TEST_QT_MAJOR_VERSION}::Test
    Qt${NYMEA_TEST_QT_MAJOR_VERSION}::Network Qt${NYMEA_TEST_QT_MAJOR_VERSION}::WebSockets)
add_test(NAME everest-connection COMMAND $<TARGET_FILE:tst_everestconnection>)
