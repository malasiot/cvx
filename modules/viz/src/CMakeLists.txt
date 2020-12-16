FIND_PACKAGE(Eigen3 REQUIRED)
FIND_PACKAGE(Assimp REQUIRED)

FIND_PACKAGE(ZLIB REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(glfw3 3 REQUIRED)
FIND_PACKAGE(OpenCV REQUIRED)

FIND_PACKAGE(Fontconfig REQUIRED)
FIND_PACKAGE(Freetype REQUIRED)
FIND_PACKAGE(HarfBuzz REQUIRED)

FIND_PACKAGE(Bullet)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5Widgets 5.5 CONFIG REQUIRED Core Widgets)
find_package(Qt5Widgets REQUIRED)


set ( LIBRARY_FOLDER "viz")
get_filename_component(LIBRARY_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../include/cvx/${LIBRARY_FOLDER}/" ABSOLUTE)

include_directories(
    ${CVX_INCLUDE_DIRS}
    3rdparty/
    ${EIGEN3_INCLUDE_DIR}
    ${GLFW3_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIR}
    ${FREETYPE_INCLUDE_DIRS}
    ${HARFBUZZ_INCLUDE_DIRS}
    ${FONTCONFIG_INCLUDE_DIR}
    ${BULLET_INCLUDE_DIRS}
)

set(LIB_SOURCES
    renderer/gl/shader.cpp
    renderer/gl/ogl_shaders.cpp
    renderer/gl/gl3w.c
    renderer/renderer.cpp
    renderer/renderer_impl.cpp
    renderer/font_manager.cpp
    renderer/font_manager.hpp
    renderer/glyph_cache.cpp
    renderer/glyph_cache.hpp
    renderer/text_item.cpp
    renderer/text_item.hpp
    renderer/mesh_data.cpp
    renderer/mesh_data.hpp
    renderer/shadow_map.cpp
    renderer/shadow_map.hpp
    renderer/text.cpp


    scene/scene.cpp
    scene/assimp_loader.cpp
    scene/camera.cpp
    scene/mesh.cpp
    scene/material.cpp

    scene/texture.cpp
    scene/geometry.cpp
    scene/intersect.cpp
    scene/intersect.hpp
    scene/octree.hpp
    scene/octree.cpp
    scene/marker.cpp

    gui/trackball.cpp
    gui/glfw_window.cpp
    gui/glfw_viewer.cpp
    gui/offscreen.cpp
    gui/simple_qt_viewer.cpp
    
    3rdparty/pugixml/pugixml.cpp
    3rdparty/pugixml/pugixml.hpp

    robot/robot_scene.cpp
    robot/urdf_loader.cpp
    robot/urdf_robot.cpp


    image/widget.cpp
    image/view.cpp
    image/tools.cpp
    image/tools_p.hpp

    ${LIBRARY_INCLUDE_DIR}/scene/node.hpp
    ${LIBRARY_INCLUDE_DIR}/scene/drawable.hpp
    ${LIBRARY_INCLUDE_DIR}/scene/geometry.hpp
    ${LIBRARY_INCLUDE_DIR}/scene/mesh.hpp
    ${LIBRARY_INCLUDE_DIR}/scene/camera.hpp
    ${LIBRARY_INCLUDE_DIR}/scene/light.hpp
    ${LIBRARY_INCLUDE_DIR}/scene/material.hpp
    ${LIBRARY_INCLUDE_DIR}/scene/marker.hpp

    ${LIBRARY_INCLUDE_DIR}/gui/trackball.hpp
    ${LIBRARY_INCLUDE_DIR}/gui/glfw_window.hpp
    ${LIBRARY_INCLUDE_DIR}/gui/glfw_viewer.hpp
    ${LIBRARY_INCLUDE_DIR}/gui/offscreen.hpp
    ${LIBRARY_INCLUDE_DIR}/gui/simple_qt_viewer.hpp
   
    ${LIBRARY_INCLUDE_DIR}/robot/robot_scene.hpp
    ${LIBRARY_INCLUDE_DIR}/robot/urdf_loader.hpp

    ${LIBRARY_INCLUDE_DIR}/renderer/renderer.hpp
    ${LIBRARY_INCLUDE_DIR}/renderer/text.hpp
    ${LIBRARY_INCLUDE_DIR}/renderer/gl/shader.hpp

    ${LIBRARY_INCLUDE_DIR}/physics/world.hpp

    ${LIBRARY_INCLUDE_DIR}/image/widget.hpp
    ${LIBRARY_INCLUDE_DIR}/image/tools.hpp
    ${LIBRARY_INCLUDE_DIR}/image/view.hpp
)

IF ( BULLET_FOUND )
ADD_DEFINITIONS(-DBT_USE_DOUBLE_PRECISION=1)
LIST(APPEND LIB_SOURCES
    physics/world.cpp
    physics/rigid_body.cpp
    physics/constraints.cpp
    physics/collision.cpp
    physics/multi_body.cpp
    physics/sensor.cpp
    physics/soft_body.cpp

    ${LIBRARY_INCLUDE_DIR}/physics/world.hpp
    ${LIBRARY_INCLUDE_DIR}/physics/rigid_body.hpp
    ${LIBRARY_INCLUDE_DIR}/physics/collision.hpp
    ${LIBRARY_INCLUDE_DIR}/physics/convert.hpp
    ${LIBRARY_INCLUDE_DIR}/physics/constraints.hpp
    ${LIBRARY_INCLUDE_DIR}/physics/multi_body.hpp
    ${LIBRARY_INCLUDE_DIR}/physics/sensor.hpp
    ${LIBRARY_INCLUDE_DIR}/physics/soft_body.hpp
)
ENDIF( BULLET_FOUND )

SET ( GFX_SOURCES
    gfx/path.cpp
    gfx/pen.cpp
    gfx/color.cpp
    gfx/image.cpp
    gfx/text_layout.cpp

    gfx/backends/cairo/canvas.cpp
    gfx/backends/cairo/text_layout_engine.cpp
    gfx/backends/cairo/text_layout_engine.hpp
    gfx/backends/cairo/font_manager.cpp
    gfx/backends/cairo/font_manager.hpp
    gfx/backends/cairo/scrptrun.cpp
    gfx/backends/cairo/text_path.cpp

    gfx/svg/svg_parser.cpp
    gfx/svg/svg_document.cpp
    gfx/svg/svg_dom.cpp
    gfx/svg/svg_length.cpp
    gfx/svg/svg_parser.hpp
    gfx/svg/svg_length.hpp
    gfx/svg/svg_style.cpp
    gfx/svg/svg_style.hpp
    gfx/svg/svg_style_attributes.hpp
    gfx/svg/svg_parse_util.cpp
    gfx/svg/svg_parse_util.hpp
    gfx/svg/svg_render_context.cpp
    gfx/svg/svg_render_context.hpp

    ${LIBRARY_INCLUDE_DIR}/gfx/backends/cairo/canvas.hpp

    ${LIBRARY_INCLUDE_DIR}/gfx/font.hpp
    ${LIBRARY_INCLUDE_DIR}/gfx/path.hpp
    ${LIBRARY_INCLUDE_DIR}/gfx/canvas.hpp
    ${LIBRARY_INCLUDE_DIR}/gfx/image.hpp
    ${LIBRARY_INCLUDE_DIR}/gfx/vector.hpp
    ${LIBRARY_INCLUDE_DIR}/gfx/rectangle.hpp
    ${LIBRARY_INCLUDE_DIR}/gfx/xform.hpp
    ${LIBRARY_INCLUDE_DIR}/gfx/text_layout.hpp
    ${LIBRARY_INCLUDE_DIR}/gfx/glyph.hpp
    ${LIBRARY_INCLUDE_DIR}/gfx/svg_document.hpp
)

QT5_ADD_RESOURCES(RCC_SRCS "image/mdi.qrc")

add_library(cvx_viz ${LIBRARY_TYPE} ${LIB_SOURCES} ${RCC_SRCS})

target_link_libraries(cvx_viz cvx_util
    Qt5::Widgets
    ${ASSIMP_LIBRARIES}
    ${OPENGL_LIBRARIES}
    glfw
    ${GLFW_LIBRARIES}
    ${OpenCV_LIBRARIES}
    ${HARFBUZZ_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${BULLET_LIBRARIES})

add_executable(iview
    image/iview/main_window.cpp
    image/iview/main_window.hpp
    image/iview/iview.cpp
)

target_link_libraries(iview cvx_viz cvx_util
    Qt5::Widgets
    ${OpenCV_LIBRARIES}

)

# Install library
install(TARGETS cvx_viz iview
  EXPORT ${PROJECT_EXPORT}
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT stlib
  COMPONENT dev)


# Install headers
install(DIRECTORY "${LIBRARY_INCLUDE_DIR}" DESTINATION "${INSTALL_INCLUDE_DIR}/cvx" )


