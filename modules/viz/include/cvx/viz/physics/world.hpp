#ifndef CVX_VIZ_PHYSICS_WORLD_HPP
#define CVX_VIZ_PHYSICS_WORLD_HPP

#include <cvx/viz/scene/node.hpp>

#include <memory>

#include <bullet/btBulletCollisionCommon.h>
#include <bullet/btBulletDynamicsCommon.h>
#include <bullet/BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <bullet/BulletSoftBody/btSoftBodyHelpers.h>
#include <bullet/BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <bullet/BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>

#include <cvx/viz/physics/collision.hpp>
#include <cvx/viz/physics/rigid_body.hpp>
#include <cvx/viz/physics/constraints.hpp>
#include <cvx/viz/scene/camera.hpp>

namespace cvx { namespace viz {


class PhysicsWorld {
public:
    PhysicsWorld() ;
    ~PhysicsWorld() ;

    void createDefaultDynamicsWorld();
    btDynamicsWorld* getDynamicsWorld();

    void stepSimulation(float deltaTime);

    CollisionShape createBoxShape(const Eigen::Vector3f &halfExtents);
    CollisionShape createCylinderShape(float radius, float len);

    bool contactTest(const RigidBody &b1) ;

    void deleteRigidBody(btRigidBody *body);

    void addBody(const RigidBody &body);
    void addConstraint(const Constraint &c);

private:

    void addCollisionShape(const btCollisionShape *shape);

    btAlignedObjectArray<const btCollisionShape *> collision_shapes_ ;
    std::unique_ptr<btBroadphaseInterface> broadphase_ ;
    std::unique_ptr<btCollisionDispatcher> dispatcher_ ;
    std::unique_ptr<btConstraintSolver> solver_ ;
    std::unique_ptr<btDefaultCollisionConfiguration> collision_config_ ;
    std::unique_ptr<btDynamicsWorld> dynamics_world_;
};

// MotionState for dynamic objects that updates the transform of the associated node in the scene
class UpdateSceneMotionState: public btMotionState {
    public:

    UpdateSceneMotionState (const cvx::viz::NodePtr &node);

    virtual void getWorldTransform( btTransform& centerOfMassWorldTrans ) const override;
    virtual void setWorldTransform( const btTransform& centerOfMassWorldTrans ) override;

private:

    btTransform	world_trans_;
    cvx::viz::NodePtr node_ ;
};

class RayPicker {
public:
    RayPicker(PhysicsWorld &world): world_(world.getDynamicsWorld()) {}

    bool movePickedBody(const Ray &ray) ;

    bool pickBody(const Ray &ray) ;

    void removePickingConstraint() ;

private:

    btDynamicsWorld *world_ ;
    btRigidBody* picked_body_ = nullptr;
    btTypedConstraint* picked_constraint_ = nullptr;
    int saved_state_;
    btVector3 old_picking_pos_;
    btVector3 hit_pos_;
    btScalar old_picking_dist_;
};


} // namespace viz
} // namepsace cvx

#endif
