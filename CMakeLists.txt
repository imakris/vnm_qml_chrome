cmake_minimum_required(VERSION 3.21)

project(vnm_qml_chrome LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickLayouts
    QuickControls2)

if(PROJECT_IS_TOP_LEVEL)
    set(VNM_QML_CHROME_BUILD_TESTS_DEFAULT ON)
    set(VNM_QML_CHROME_BUILD_EXAMPLES_DEFAULT ON)
else()
    set(VNM_QML_CHROME_BUILD_TESTS_DEFAULT OFF)
    set(VNM_QML_CHROME_BUILD_EXAMPLES_DEFAULT OFF)
endif()

option(
    VNM_QML_CHROME_BUILD_TESTS
    "Build vnm_qml_chrome standalone contract tests"
    ${VNM_QML_CHROME_BUILD_TESTS_DEFAULT})
option(
    VNM_QML_CHROME_BUILD_EXAMPLES
    "Build vnm_qml_chrome standalone examples"
    ${VNM_QML_CHROME_BUILD_EXAMPLES_DEFAULT})

add_library(vnm_qml_chrome STATIC)
add_library(vnm_qml_chrome::vnm_qml_chrome ALIAS vnm_qml_chrome)

target_sources(vnm_qml_chrome PRIVATE
    include/vnm_qml_chrome/vnm_qml_chrome_runtime.h
    include/vnm_qml_chrome/vnm_native_window_frame.h
    src/vnm_native_window_frame.cpp
    src/vnm_qml_chrome_runtime.cpp
    vnm_qml_chrome.qrc
)

target_include_directories(vnm_qml_chrome
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(vnm_qml_chrome
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickLayouts
        Qt6::QuickControls2
)

if(WIN32)
    target_compile_definitions(vnm_qml_chrome PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(vnm_qml_chrome PRIVATE gdi32 user32)
endif()

if(VNM_QML_CHROME_BUILD_TESTS)
    include(CTest)
    find_package(Qt6 REQUIRED COMPONENTS Test)

    add_executable(vnm_chrome_contract_tests
        tests/vnm_chrome_contract_tests.cpp
    )

    target_link_libraries(vnm_chrome_contract_tests PRIVATE
        vnm_qml_chrome::vnm_qml_chrome
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickLayouts
        Qt6::QuickControls2
        Qt6::Test
    )

    if(BUILD_TESTING)
        add_test(
            NAME vnm_chrome_contract_tests
            COMMAND ${CMAKE_COMMAND} -E env
                "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
                "$<TARGET_FILE:vnm_chrome_contract_tests>"
        )
    endif()
endif()

if(VNM_QML_CHROME_BUILD_EXAMPLES)
    add_executable(vnm_qml_chrome_solid_window_example
        examples/solid_window/main.cpp
        examples/solid_window/solid_window.qrc
    )

    target_link_libraries(vnm_qml_chrome_solid_window_example PRIVATE
        vnm_qml_chrome::vnm_qml_chrome
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
    )
endif()
