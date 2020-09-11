find_package(Bullet)

IF (BULLET_FOUND) 
include_directories(
    ${CVX_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    ${BULLET_INCLUDE_DIRS}
)

ADD_DEFINITIONS(-DBT_USE_DOUBLE_PRECISION=1)

add_executable(test_bullet test_bullet.cpp bullet_gui.cpp bullet_gui.hpp
body.cpp body.hpp)
target_link_libraries(test_bullet cvx_viz ${BULLET_LIBRARIES})

add_executable(test_rope test_rope.cpp bullet_gui.cpp bullet_gui.hpp)
set_target_properties(test_rope PROPERTIES AUTOMOC TRUE)
target_link_libraries(test_rope cvx_viz ${BULLET_LIBRARIES})

add_executable(test_gripper test_gripper.cpp bullet_gui.cpp bullet_gui.hpp)
set_target_properties(test_gripper PROPERTIES AUTOMOC TRUE)
target_link_libraries(test_gripper cvx_viz ${BULLET_LIBRARIES})

add_executable(test_bullet_robot test_robot.cpp bullet_gui.cpp bullet_gui.hpp)
set_target_properties(test_bullet_robot PROPERTIES AUTOMOC TRUE)
target_link_libraries(test_bullet_robot cvx_viz ${BULLET_LIBRARIES})

#add_executable(test_cloth test_cloth.cpp bullet_gui.cpp bullet_gui.hpp)
#set_target_properties(test_cloth PROPERTIES AUTOMOC TRUE)
#target_link_libraries(test_cloth cvx_viz ${BULLET_LIBRARIES})

ENDIF(BULLET_FOUND)
