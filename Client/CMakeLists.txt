project(Client LANGUAGES CXX)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(../common)
find_package(Qt5 5.7 COMPONENTS
        Core
        Gui
        Widgets
        Network
        REQUIRED)
aux_source_directory(../Client SOURCES)
aux_source_directory(../common SOURCES)
set(RESOURCES resources/logo.rc resources/resources.qrc)

add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network)

add_compile_definitions(QT_MESSAGELOGCONTEXT)
set_target_properties(${PROJECT_NAME} PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        VERSION "1.0.0"
        EXPORT_NAME Client
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

