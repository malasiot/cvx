#ifndef CVX_GEOM_UTIL_HPP
#define CVX_GEOM_UTIL_HPP

#include <Eigen/Geometry>
#include <vector>

namespace cvx {

Eigen::Matrix4f lookAt(const Eigen::Vector3f & eye,  const Eigen::Vector3f &center, const Eigen::Vector3f &up) ;

Eigen::Hyperplane<float, 3> fitPlaneToPoints(const std::vector<Eigen::Vector3f> &pts) ;

// Kabsch algorithm for rigid pose estimation between two point clouds ( finds T to minimize sum_i ||P_i * T - Q_i|| )

Eigen::Isometry3f find_rigid(const Eigen::Matrix3Xf &P, const Eigen::Matrix3Xf &Q) ;
Eigen::Isometry3f find_rigid(const std::vector<Eigen::Vector3f> &P, const std::vector<Eigen::Vector3f> &Q) ;

}

#endif
