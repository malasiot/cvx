add_executable(make_osm_graph make_osm_graph.cpp osm_parser.cpp)
target_link_libraries(make_osm_graph cvx_util cvx_db)


