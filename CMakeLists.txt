cmake_minimum_required(VERSION 3.16)
project(wicky-monads VERSION 0.1)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

configure_file(config.h.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

include(FetchContent)

FetchContent_Declare(json
        GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
        GIT_TAG v3.9.1)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
    FetchContent_Populate(json)
    add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_executable(wic wic.c common.c)
add_executable(monad monad.cxx common.c)
target_link_libraries(monad nlohmann_json::nlohmann_json)
target_link_libraries(monad rt)

if(NOT EXISTS ${PANDOC_EXECUTABLE})
    find_program(PANDOC_EXECUTABLE pandoc)
    mark_as_advanced(PANDOC_EXECUTABLE)
    if(NOT EXISTS ${PANDOC_EXECUTABLE})
        message(FATAL_ERROR "Pandoc not found. Install Pandoc or set cache variable PANDOC_EXECUTABLE.")
    endif()
endif()
add_custom_command(OUTPUT wic.1
        COMMAND pandoc ARGS -s -t man -o wic.1 ${CMAKE_CURRENT_SOURCE_DIR}/man/wic.md
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/man/wic.md
        COMMENT "Using pandoc to generate man page: wic.1")
add_custom_command(OUTPUT monad.1
        COMMAND pandoc ARGS -s -t man -o monad.1 ${CMAKE_CURRENT_SOURCE_DIR}/man/monad.md
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/man/monad.md
        COMMENT "Using pandoc to generate man page: monad.1")
add_custom_target(manual DEPENDS wic.1 monad.1)

install(TARGETS wic DESTINATION bin)
install(TARGETS monad DESTINATION bin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/wic.1 DESTINATION share/man/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/monad.1 DESTINATION share/man/)
install(FILES LICENSE DESTINATION share/licenses/wicky-monads/)
