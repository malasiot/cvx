FIND_PACKAGE(Cairo REQUIRED)
FIND_PACKAGE(Fontconfig REQUIRED)
FIND_PACKAGE(Freetype REQUIRED)
FIND_PACKAGE(HarfBuzz REQUIRED)
FIND_PACKAGE(ICU REQUIRED COMPONENTS uc i18n)

option(BUILD_GFX_QT_SURFACE "Build QtPainter graphics surface" OFF)

IF( BUILD_GFX_QT_SURFACE )
find_package(Qt5Widgets 5.5 CONFIG REQUIRED)
SET(QT_LIBRARIES Qt5::Widgets)
ENDIF (BUILD_GFX_QT_SURFACE )

set ( LIBRARY_FOLDER "gfx")
get_filename_component(LIBRARY_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../include/cvx/${LIBRARY_FOLDER}/" ABSOLUTE)

include_directories(
    ${CVX_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${HARFBUZZ_INCLUDE_DIRS}
    ${FONTCONFIG_INCLUDE_DIR}
)

set(LIB_SOURCES
   path.cpp
    pen.cpp
    color.cpp
    image.cpp
    text_layout.cpp

    impl/canvas.cpp
    impl/text_layout_engine.cpp
    impl/text_layout_engine.hpp
    impl/font_manager.cpp
    impl/font_manager.hpp
    impl/scrptrun.cpp
    impl/text_path.cpp
    impl/surface.cpp

    ${LIBRARY_INCLUDE_DIR}/impl/canvas.hpp

    ${LIBRARY_INCLUDE_DIR}/font.hpp
    ${LIBRARY_INCLUDE_DIR}/path.hpp
    ${LIBRARY_INCLUDE_DIR}/canvas.hpp
    ${LIBRARY_INCLUDE_DIR}/image.hpp
    ${LIBRARY_INCLUDE_DIR}/vector.hpp
    ${LIBRARY_INCLUDE_DIR}/rectangle.hpp
    ${LIBRARY_INCLUDE_DIR}/xform.hpp
    ${LIBRARY_INCLUDE_DIR}/text_layout.hpp
    ${LIBRARY_INCLUDE_DIR}/glyph.hpp

    svg/svg_parser.cpp
    svg/svg_parser.hpp
    svg/svg_document.cpp
    svg/svg_dom.cpp
    svg/svg_length.cpp
    svg/svg_length.hpp
    svg/svg_style.cpp
    svg/svg_style.hpp
    svg/svg_style_attributes.hpp
    svg/svg_parse_util.cpp
    svg/svg_parse_util.hpp
    svg/svg_render_context.cpp
    svg/svg_render_context.hpp

    ${LIBRARY_INCLUDE_DIR}/svg_document.hpp

    plot/axis.cpp
    plot/line_graph.cpp
    plot/plot.cpp
    plot/tics.cpp
    plot/markers.cpp
    plot/legend.cpp

)

IF ( BUILD_GFX_QT_SURFACE )
LIST(APPEND LIB_SOURCES qt_surface.cpp)
ENDIF()

add_library(cvx_gfx ${LIBRARY_TYPE} ${LIB_SOURCES} )

target_link_libraries(cvx_gfx cvx_util
    ${CAIRO_LIBRARIES}
    ${HARFBUZZ_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${ICU_LIBRARIES}
    ${QT_LIBRARIES}
    )

# Install library
install(TARGETS cvx_gfx
  EXPORT ${PROJECT_EXPORT}
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT stlib
  COMPONENT dev)


# Install headers
install(DIRECTORY "${LIBRARY_INCLUDE_DIR}" DESTINATION "${INSTALL_INCLUDE_DIR}/cvx" )


