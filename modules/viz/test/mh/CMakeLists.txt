include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/vcglib/)
add_executable(test_mh
    test_mh.cpp
    mhx2_importer.cpp
    mh_scene.cpp
    skinned_mesh.cpp
    pose.cpp
    skeleton.cpp
    mesh_utils.cpp
    vcg_mesh.cpp)
target_link_libraries(test_mh cvx_util cvx_viz)

