cmake_minimum_required(VERSION 3.16)

project(MatchingEngine
    VERSION 0.1.0
    DESCRIPTION "C++ matching engine"
    LANGUAGES CXX)

# Global C++ 

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release if no build type set
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build (Debug, Release, RelWithDebInfo, MinSizeRel)."
        FORCE)
endif()



# Some reasonable warning flags
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Library with core matching engine code  #

add_library(matching_engine STATIC
    src/engine.cpp
    src/order_book.cpp
)

target_include_directories(matching_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)


if (NOT MSVC)
  target_compile_options(matching_engine PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -DNDEBUG>
    )
    target_link_options(matching_engine PRIVATE
      $<$<CONFIG:Release>: -flto>
    )
endif()


# main_demo executable

add_executable(main_demo
    src/main_demo.cpp
)

target_link_libraries(main_demo
    PRIVATE
        matching_engine
)

# test_engine executable

add_executable(test_engine
    src/test_engine.cpp
)

target_link_libraries(test_engine
    PRIVATE
        matching_engine
)

# Optional: organize targets in an IDE
set_target_properties(matching_engine PROPERTIES OUTPUT_NAME "matching_engine")
set_target_properties(main_demo    PROPERTIES OUTPUT_NAME "main_demo")
set_target_properties(test_engine  PROPERTIES OUTPUT_NAME "test_engine")
