find_package(OpenNI2)

IF (OPENNI2_FOUND) 
include_directories(
    ${CVX_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    ${OPENNI2_INCLUDE_DIRS}
)

add_executable(rgbd_viewer openni_grabber.cpp openni_grabber.hpp rgbd_viewer.hpp rgbd_viewer.cpp)
target_link_libraries(rgbd_viewer cvx_viz ${OPENNI2_LIBRARIES})

ENDIF(OPENNI2_FOUND)
