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
#include <bullet/BulletDynamics/Featherstone/btMultiBodyPoint2Point.h>

#include <cvx/viz/physics/collision.hpp>
#include <cvx/viz/physics/rigid_body.hpp>
#include <cvx/viz/physics/multi_body.hpp>
#include <cvx/viz/physics/constraints.hpp>
#include <cvx/viz/scene/camera.hpp>

namespace cvx { namespace viz {

struct ContactResult {

    CollisionObject *a_, *b_ ;
    Eigen::Vector3f pa_, pb_, normal_ ;
} ;

struct RayHitResult {
    CollisionObject *o_ ; // hit object
    Eigen::Vector3f p_, n_ ; // position and normal of hit in world coordinates
};

class PhysicsWorld {
public:
    PhysicsWorld() ;
    ~PhysicsWorld() ;

    void createDefaultDynamicsWorld();
    void createMultiBodyDynamicsWorld();

    btDynamicsWorld* getDynamicsWorld();

    void setGravity(const Eigen::Vector3f &g) ;

    void stepSimulation(float deltaTime);

    bool contactTest(const RigidBodyPtr &b1, std::vector<ContactResult> &results) ;

    bool rayPick(const Eigen::Vector3f &origin, const Eigen::Vector3f &dir, RayHitResult &res);

    // return the index of the object in the internal list
    uint addBody(const RigidBodyPtr &body);
    uint addMultiBody(const MultiBodyPtr &body) ;

    void addConstraint(const Constraint &c);

    RigidBodyPtr getRigidBody(uint idx) const ;
    MultiBodyPtr getMultiBody(uint idx) const ;

    RigidBodyPtr findRigidBody(const std::string &name) ;
    MultiBodyPtr findMultiBody(const std::string &name) ;
private:

    void addCollisionShape(const btCollisionShape *shape);

    btAlignedObjectArray<const btCollisionShape *> collision_shapes_ ;
    std::unique_ptr<btBroadphaseInterface> broadphase_ ;
    std::unique_ptr<btCollisionDispatcher> dispatcher_ ;
    std::unique_ptr<btConstraintSolver> solver_ ;
    std::unique_ptr<btDefaultCollisionConfiguration> collision_config_ ;
    std::unique_ptr<btDynamicsWorld> dynamics_world_;

    std::vector<RigidBodyPtr> bodies_ ;
    std::map<std::string, uint> body_map_ ;
    std::vector<MultiBodyPtr> multi_bodies_ ;
    std::map<std::string, uint> multi_body_map_ ;
    std::vector<Constraint> constraints_ ;
};

// MotionState for dynamic objects that updates the transform of the associated node in the scene
class UpdateSceneMotionState: public btMotionState {
    public:

    UpdateSceneMotionState (const cvx::viz::NodePtr &node);
    UpdateSceneMotionState (const cvx::viz::NodePtr &node, const btTransform &local);

    virtual void getWorldTransform( btTransform& centerOfMassWorldTrans ) const override;
    virtual void setWorldTransform( const btTransform& centerOfMassWorldTrans ) override;

private:

    btTransform	world_trans_, local_frame_;
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
    btTypedConstraint* picked_constraint_ = nullptr ;
    btMultiBodyPoint2Point *mb_picked_constraint_ = nullptr ;
    int saved_state_;
    btVector3 old_picking_pos_;
    btVector3 hit_pos_;
    btScalar old_picking_dist_;
    bool prev_can_sleep_ ;
};


} // namespace viz
} // namepsace cvx

#endif
