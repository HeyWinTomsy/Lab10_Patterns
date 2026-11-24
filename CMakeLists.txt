cmake_minimum_required(VERSION 3.21)

project(Lab10_CallCenter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SqliteOrm CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

add_executable(Lab10_CallCenter
    src/main.cpp
    src/OperatorRepository.cpp
)

target_include_directories(Lab10_CallCenter PRIVATE include)

target_link_libraries(Lab10_CallCenter PRIVATE
    sqlite_orm::sqlite_orm
    unofficial::sqlite3::sqlite3
)

enable_testing()

add_executable(PatternTests tests/PatternTests.cpp)
target_include_directories(PatternTests PRIVATE include)
target_link_libraries(PatternTests PRIVATE
    sqlite_orm::sqlite_orm
    unofficial::sqlite3::sqlite3
)
add_test(NAME PatternTests COMMAND PatternTests)
