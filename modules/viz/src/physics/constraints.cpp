#include <cvx/viz/physics/constraints.hpp>
#include <cvx/viz/physics/convert.hpp>

namespace cvx { namespace viz {

Point2PointConstraint::Point2PointConstraint(const cvx::viz::RigidBody &b1, const cvx::viz::RigidBody &b2, const Eigen::Vector3f &pivot1, const Eigen::Vector3f &pivot2) {
    handle_.reset(new btPoint2PointConstraint (*b1.handle(), *b2.handle(), eigenVectorToBullet(pivot1), eigenVectorToBullet(pivot2))) ;
}

}}
