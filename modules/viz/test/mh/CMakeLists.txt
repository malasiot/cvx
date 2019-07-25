add_executable(test_mh test_mh.cpp mhx2_importer.cpp mh_scene.cpp skinned_mesh.cpp pose.cpp skeleton.cpp)
target_link_libraries(test_mh cvx_util cvx_viz)

