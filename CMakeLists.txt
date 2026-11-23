cmake_minimum_required(VERSION 3.20)
project(StudentTracker VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include paths
include_directories(
    backend/include
    backend/third_party
)

# Detect SQLite3: use system package if available, otherwise compile third_party amalgamation
find_package(SQLite3 QUIET)

set(BACKEND_SOURCES
    backend/src/main.cpp
    backend/src/db/DatabaseManager.cpp
    backend/src/repository/ItemRepository.cpp
    backend/src/services/ItemService.cpp
    backend/src/controllers/ItemController.cpp
)

if(TARGET SQLite::SQLite3)
    message(STATUS "Found system SQLite3")
    set(SQLITE_TARGET SQLite::SQLite3)
else()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/backend/third_party/sqlite3.c")
        message(STATUS "Using bundled SQLite3 amalgamation from backend/third_party")
        add_library(sqlite3_bundled STATIC backend/third_party/sqlite3.c)
        target_include_directories(sqlite3_bundled PUBLIC backend/third_party)
        set(SQLITE_TARGET sqlite3_bundled)
    else()
        message(WARNING "SQLite3 not found on system and not present in backend/third_party. Run scripts/setup_dependencies.ps1 to download.")
    endif()
endif()

add_executable(tracker_server ${BACKEND_SOURCES})

if(DEFINED SQLITE_TARGET)
    target_link_libraries(tracker_server PRIVATE ${SQLITE_TARGET})
endif()

# Windows socket and crypto libraries required by cpp-httplib
if(WIN32)
    target_link_libraries(tracker_server PRIVATE ws2_32 crypt32)
endif()

# Threading support required by cpp-httplib
find_package(Threads REQUIRED)
target_link_libraries(tracker_server PRIVATE Threads::Threads)

# Output directory
set_target_properties(tracker_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
