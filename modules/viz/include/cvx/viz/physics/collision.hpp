#ifndef CVX_VIZ_PHYSICS_COLLISION_HPP
#define CVX_VIZ_PHYSICS_COLLISION_HPP

#include <memory>

#include <bullet/btBulletCollisionCommon.h>
#include <Eigen/Geometry>
#include <cvx/viz/physics/convert.hpp>

namespace cvx { namespace viz {
class CollisionShape {

public:
    btCollisionShape *handle() const { return handle_ ; }

    Eigen::Vector3f computeLocalInertia(btScalar mass) {
        btVector3 inertia{0, 0, 0} ;
        handle_->calculateLocalInertia(mass, inertia) ;
        return toEigenVector(inertia) ;
    }

private:

    friend class PhysicsWorld ;

    CollisionShape(btCollisionShape *cs): handle_(cs) {}

    btCollisionShape *handle_ ;
};

} // namespace viz
} // namespace cvx

#endif
