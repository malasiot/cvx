find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(ZLIB REQUIRED)

find_package(F2C REQUIRED)
find_package(LAPACK REQUIRED)

include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../include/
   ${EIGEN3_INCLUDE_DIR}
   ${OpenCV_INCLUDE_DIRS}
)

set ( LIBRARY_FOLDER "util")
get_filename_component(LIBRARY_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../include/cvx/${LIBRARY_FOLDER}/" ABSOLUTE)

set ( LIB_SOURCES

    misc/binary_stream.cpp
    misc/application_settings.cpp
    misc/logger.cpp
    misc/dictionary.cpp
    misc/progress_stream.cpp
    misc/arg_parser.cpp
    misc/dir_iterator.cpp
    misc/path.cpp
    misc/base64.cpp

    misc/strings.cpp
    misc/zstream.cpp
    misc/cv_helpers.cpp
    misc/sequence.cpp
    misc/filesystem.cpp
    misc/xml_pull_parser.cpp
    misc/xml_sax_parser.cpp
    misc/xml_writer.cpp
    misc/json_reader.cpp
    misc/json_writer.cpp
    misc/format.cpp

    geometry/trimesh_topology.cpp
    3rdparty/triangle.c
    3rdparty/nanoflann.hpp
    geometry/triangulate.cpp
    geometry/polygon_scanner.cpp
    geometry/kdtree.cpp
    geometry/octree.cpp
    geometry/util.cpp
    geometry/viewpoint_sampler.cpp

    viz/gnuplot.cpp

    imgproc/rgbd.cpp
    imgproc/concomp.cpp
    imgproc/gabor.cpp

    pcl/align.cpp
    pcl/icp.cpp

    math/rng.cpp
    math/lm_impl.cpp
)

if ( UNIX )
    list(APPEND LIB_SOURCES misc/path_unix.cpp misc/dir_iterator_unix.cpp)
endif ( UNIX )

set ( LIB_HEADERS

    misc/binary_stream.hpp
    misc/application_settings.hpp
    misc/logger.hpp
    misc/dictionary.hpp
    misc/progress_stream.hpp
    misc/arg_parser.hpp
    misc/dir_iterator.hpp
    misc/path.hpp

    misc/strings.hpp
    misc/zstream.hpp
    misc/cv_helpers.hpp
    misc/parallel.hpp
    misc/sequence.hpp
    misc/filesystem.hpp
    misc/optional.hpp
    misc/xml_pull_parser.hpp
    misc/xml_writer.hpp
    misc/json_reader.hpp
    misc/json_writer.hpp
    misc/variant.hpp
    misc/base64.hpp

    geometry/point.hpp
    geometry/point_list.hpp
    geometry/line.hpp
    geometry/line_fit.hpp
    geometry/rectangle.hpp
    geometry/trimesh_topology.hpp
    geometry/polygon.hpp
    geometry/triangle.hpp
    geometry/triangulate.hpp
    geometry/polygon_scanner.hpp
    geometry/kdtree.hpp
    geometry/octree.hpp
    geometry/util.hpp
    geometry/viewpoint_sampler.hpp

    camera/camera.hpp

    viz/gnuplot.hpp

    math/solvers/bfgs.hpp
    math/solvers/lbfgs.hpp
    math/solvers/gradient_descent.hpp
    math/solvers/lm.hpp
    math/ransac.hpp
    math/rng.hpp

    imgproc/rgbd.hpp
    imgproc/concomp.hpp
    imgproc/gabor.hpp

    pcl/align.hpp
    pcl/icp.hpp
)

SET ( LIB_HEADERS_ABS )
FOREACH ( file ${LIB_HEADERS} )
    LIST(APPEND LIB_HEADERS_ABS "${LIBRARY_INCLUDE_DIR}/${file}")
ENDFOREACH ()

include_directories(3rdparty)

add_subdirectory(3rdparty/levmar)


set_source_files_properties(rgbd/depth_filter/jointNearest.cpp
                            rgbd/depth_filter/split.cpp
                            PROPERTIES COMPILE_FLAGS -msse4.1)

#linking 

add_library(cvx_util ${LIBRARY_TYPE} ${LIB_SOURCES} ${LIB_HEADERS_ABS})

TARGET_LINK_LIBRARIES(cvx_util
    PUBLIC
        ${OpenCV_LIBS}
        levmar
        ${LAPACK_LIBRARIES}
        ${F2C_LIBRARIES}
        ${ZLIB_LIBRARIES}

)

# Install library
install(TARGETS cvx_util
  EXPORT ${PROJECT_EXPORT}
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT stlib
  COMPONENT dev)

# Install headers
install(DIRECTORY "${LIBRARY_INCLUDE_DIR}" DESTINATION "${INSTALL_INCLUDE_DIR}/cvx" )

