find_package(Bullet)

IF (BULLET_FOUND) 
include_directories(
    ${CVX_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    ${BULLET_INCLUDE_DIRS}
)

add_executable(test_bullet test_bullet.cpp collision_world.cpp)
target_link_libraries(test_bullet cvx_viz ${BULLET_LIBRARIES})
ENDIF(BULLET_FOUND)
