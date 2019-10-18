include(ExternalProject)

ExternalProject_Add(spatialite
  URL               http://www.gaia-gis.it/gaia-sins/libspatialite-4.3.0a.tar.gz
  SOURCE_DIR        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/spatialite"
  CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/spatialite/./configure --enable-freexl=no --prefix=${CMAKE_BINARY_DIR}
  INSTALL_DIR       ${CMAKE_BINARY_DIR}
  BUILD_COMMAND     make -j10
)

SET(SPATIALITE_LIBRARY ${CMAKE_BINARY_DIR}/lib/libspatialite.so )

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/spatialite
)

add_executable(make_osm_graph make_osm_graph.cpp osm_parser.cpp dem.cpp graph.cpp)
target_link_libraries(make_osm_graph cvx_util cvx_db sqlite3 ${SPATIALITE_LIBRARY})

add_executable(make_osm_pois make_osm_pois.cpp osm_parser.cpp poi_writer.cpp poi_config.cpp dem.cpp)
target_link_libraries(make_osm_pois cvx_util cvx_db sqlite3 ${SPATIALITE_LIBRARY})
