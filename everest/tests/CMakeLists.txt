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
