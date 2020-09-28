#ifndef CVX_VIZ_PHYSICS_RIGID_BODY_HPP
#define CVX_VIZ_PHYSICS_RIGID_BODY_HPP

#include <memory>


#include <bullet/btBulletCollisionCommon.h>
#include <bullet/btBulletDynamicsCommon.h>

#include <cvx/viz/physics/collision.hpp>
#include <cvx/viz/physics/convert.hpp>

namespace cvx { namespace viz {

struct RigidBodyData ;

class RigidBody: public CollisionObject {
public :
    // dynamic body with given inertia
    RigidBody(btScalar mass, btMotionState *ms, const CollisionShape::Ptr &shape, const Eigen::Vector3f &localInertia) ;

    // dynamic body with intertia computed by collision shape and mass
    RigidBody(btScalar mass, btMotionState *ms, const CollisionShape::Ptr &shape) ;

    // static body
    RigidBody(const CollisionShape::Ptr &shape, const Eigen::Affine3f &tr) ;

    btRigidBody *handle() const;

    void setName(const std::string &name);

    std::string getName() const override {
        return name_ ;
    }

private:

    friend class PhysicsWorld ;

    std::string name_ ;
    std::unique_ptr<btRigidBody> handle_ ;
    CollisionShape::Ptr collision_shape_ ;
    std::unique_ptr<btMotionState> motion_state_ ;
};

using RigidBodyPtr = std::shared_ptr<RigidBody> ;

} // namespace viz
} // namespace cvx

#endif
