FIND_PACKAGE(SPATIALITE REQUIRED)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}
        ${SPATIALITE_INCLUDE_DIR}
)

add_executable(make_osm_graph make_osm_graph.cpp osm_parser.cpp dem.cpp graph.cpp)
target_link_libraries(make_osm_graph cvx_util cvx_db sqlite3 ${SPATIALITE_LIBRARY})


