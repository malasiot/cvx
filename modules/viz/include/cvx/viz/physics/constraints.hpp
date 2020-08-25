#ifndef CVX_VIZ_PHYSICS_CONSTRAINTS_HPP
#define CVX_VIZ_PHYSICS_CONSTRAINTS_HPP

#include <bullet/BulletDynamics/btBulletDynamicsCommon.h>

#include <cvx/viz/physics/rigid_body.hpp>

namespace cvx { namespace viz {

class Constraint {
public:
    btTypedConstraint *handle() const { return handle_.get() ; }
protected:

    std::shared_ptr<btTypedConstraint> handle_ ;
};

class Point2PointConstraint: public Constraint {

public:
    Point2PointConstraint(const RigidBody &b1, const RigidBody &b2, const Eigen::Vector3f &pivot1, const Eigen::Vector3f &pivot2) ;
};

} // namespace viz
} // namespace cvx

#endif
