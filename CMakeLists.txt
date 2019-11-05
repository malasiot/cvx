cmake_minimum_required(VERSION 3.0.0)

project(cvx)

set(CMAKE_MODULE_PATH ${CMAKE_ROOT}/Modules ${CMAKE_CURRENT_SOURCE_DIR}/CMake/)

include(Setup)
include(CMakeDependentOption)
include(Modules)

find_package(OpenMP)

if (OPENMP_FOUND)
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()


if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
  if( CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9")
    find_package(Boost 1.49 REQUIRED COMPONENTS system regex)
    add_definitions(-DHAS_BOOST_REGEX)
  endif()
endif()


add_definitions(-std=c++11)

IF ( CMAKE_COMPILER_IS_GNUCXX )
	SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wreturn-type" )
ENDIF( CMAKE_COMPILER_IS_GNUCXX )


SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

# the RPATH to be used when installing
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# don't add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

DeclareModule(NAME util BUILD "ON")
DeclareModule(NAME viz BUILD "OFF" DEPENDS util)
DeclareModule(NAME db BUILD "OFF" DEPENDS util)
DeclareModule(NAME calib BUILD "OFF" DEPENDS util)
DeclareModule(NAME stereo BUILD "OFF" DEPENDS util)

option(BUILD_APPLICATIONS "build application" OFF)

if ( BUILD_APPLICATIONS )
    include(Applications)
endif()


include(${CMAKE_SOURCE_DIR}/CMake/Install.cmake)

