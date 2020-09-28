INCLUDE_DIRECTORIES(${CVX_INCLUDE_DIRS})

file(GLOB GFX_TEST_FILES "*.cpp")

IF( BUILD_GFX_QT_SURFACE )
find_package(Qt5Widgets 5.5 CONFIG REQUIRED Core Widgets)
SET(QT_LIBRARIES Qt5::Widgets)
ELSE (BUILD_GFX_QT_SURFACE )
LIST(REMOVE_ITEM GFX_TEST_FILES test_qt_surface.cpp)
ENDIF()

foreach(TEST_FILENAME ${GFX_TEST_FILES})
    get_filename_component(TEST_TARGET ${TEST_FILENAME} NAME_WE)
    add_executable(${TEST_TARGET} ${TEST_FILENAME})
    target_link_libraries(${TEST_TARGET} cvx_gfx ${QT_LIBRARIES})
endforeach(TEST_FILENAME)
