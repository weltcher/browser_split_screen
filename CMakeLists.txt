cmake_minimum_required(VERSION 3.16)

project(BrowserSplitScreen VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql WebEngineWidgets)

# WebEngineWidgets is now required
message(STATUS "Qt6WebEngineWidgets found - Web browsing enabled")
add_definitions(-DHAVE_WEBENGINE)

# Enable Qt6 automatic MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/LoginDialog.cpp
    src/BrowserWidget.cpp
    src/DatabaseManager.cpp
    src/WindowManager.cpp
    src/SubWindowManager.cpp
)

# Header files
set(HEADERS
    include/MainWindow.h
    include/LoginDialog.h
    include/BrowserWidget.h
    include/DatabaseManager.h
    include/WindowManager.h
    include/SubWindowManager.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::WebEngineWidgets
)

# Set target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# Install rules
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

set(DB_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/browser_split_screen.db")
if (EXISTS "${DB_SOURCE}")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DB_SOURCE}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/browser_split_screen.db"
        COMMENT "Copying database to output directory"
    )

    install(FILES "${DB_SOURCE}"
        DESTINATION bin
        OPTIONAL
    )
else()
    message(WARNING "Database file '${DB_SOURCE}' not found. Skipping copy/install steps.")
endif()
