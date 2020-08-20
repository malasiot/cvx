#ifndef CVX_VIZ_PHYSICS_RIGID_BODY_HPP
#define CVX_VIZ_PHYSICS_RIGID_BODY_HPP

#include <memory>


#include <bullet/btBulletCollisionCommon.h>
#include <bullet/btBulletDynamicsCommon.h>

#include <cvx/viz/physics/collision.hpp>
#include <cvx/viz/physics/convert.hpp>

namespace cvx { namespace viz {

class RigidBody {
public :
    // dynamic body with given inertia
    RigidBody(btScalar mass, btMotionState *ms, const CollisionShape &shape, const Eigen::Vector3f &localInertia) ;

    // dynamic body with intertia computed by collision shape and mass
    RigidBody(btScalar mass, btMotionState *ms, const CollisionShape &shape) ;

    // static body
    RigidBody(const CollisionShape &shape, const Eigen::Affine3f &tr) ;

    btRigidBody *handle() const { return handle_ ; }

private:

    friend class PhysicsWorld ;

    RigidBody(btRigidBody *cs): handle_(cs) {}

    btRigidBody *handle_ ;
};

} // namespace viz
} // namespace cvx

#endif
