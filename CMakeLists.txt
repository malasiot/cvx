cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(cvx VERSION 1.0 LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(setup)
include(dependencies)

add_subdirectory(src)

if (BUILD_TESTS)
add_subdirectory(test)
endif ()

include(install)


