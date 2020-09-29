#include <cvx/viz/physics/world.hpp>
#include <cvx/viz/physics/multi_body.hpp>
#include <cvx/viz/physics/convert.hpp>
#include <bullet/BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <bullet/BulletCollision/CollisionDispatch/btGhostObject.h>
#include <bullet/BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h>
#include <bullet/BulletDynamics/ConstraintSolver/btNNCGConstraintSolver.h>
#include <bullet/BulletDynamics/MLCPSolvers/btMLCPSolver.h>
#include <bullet/BulletDynamics/MLCPSolvers/btSolveProjectedGaussSeidel.h>
#include <bullet/BulletDynamics/MLCPSolvers/btLemkeSolver.h>
#include <bullet/BulletDynamics/MLCPSolvers/btDantzigSolver.h>
#include <bullet/BulletDynamics/Featherstone/btMultiBodyConstraintSolver.h>
#include <bullet/BulletDynamics/Featherstone/btMultiBodyDynamicsWorld.h>

using namespace std ;
using namespace Eigen ;

namespace cvx { namespace viz {

PhysicsWorld::PhysicsWorld() {}

void PhysicsWorld::tickCallback(btDynamicsWorld *world, btScalar step) {
    PhysicsWorld *w = static_cast<PhysicsWorld *>(world->getWorldUserInfo()) ;
    w->updateSimTime(step) ;

    w->queryCollisions() ;
    for( SensorPtr sensor: w->sensors_ )
        sensor->update(w->getSimTime()) ;

}

void PhysicsWorld::updateSimTime(float step) {
    sim_time_ += step ;
}

float PhysicsWorld::getSimTime() const {
    return sim_time_ ;
}

void PhysicsWorld::createDefaultDynamicsWorld() {
    collision_config_.reset( new btDefaultCollisionConfiguration() ) ;

    ///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
    dispatcher_.reset( new btCollisionDispatcher(collision_config_.get()) ) ;

    //  broadphase_.reset(new btDbvtBroadphase() ) ;

    btVector3 worldAabbMin(-10000, -10000, -10000);
    btVector3 worldAabbMax(10000, 10000, 10000);
    broadphase_.reset(new btAxisSweep3(worldAabbMin, worldAabbMax));


    // solver_.reset(new btNNCGConstraintSolver());
    //solver_.reset(new btMLCPSolver(new btSolveProjectedGaussSeidel()));
    solver_.reset(new btMLCPSolver(new btDantzigSolver()));
    //m_solver = new btMLCPSolver(new btLemkeSolver());

    solver_.reset( new btSequentialImpulseConstraintSolver() ) ;

    dynamics_world_.reset( new btDiscreteDynamicsWorld(dispatcher_.get(), broadphase_.get(), solver_.get(), collision_config_.get()));
    dynamics_world_->getDispatchInfo().m_useContinuous = true;

    dynamics_world_->setGravity(btVector3(0, -10, 0));

    dynamics_world_->setInternalTickCallback(tickCallback, static_cast<void *>(this)) ;

    broadphase_->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());

}



void PhysicsWorld::createMultiBodyDynamicsWorld()
{
    collision_config_.reset( new btDefaultCollisionConfiguration() ) ;

    ///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
    dispatcher_.reset( new btCollisionDispatcher(collision_config_.get()) ) ;

    broadphase_.reset(new btDbvtBroadphase() ) ;

    solver_.reset(new btMultiBodyConstraintSolver()) ;

    dynamics_world_.reset( new btMultiBodyDynamicsWorld(dispatcher_.get(), broadphase_.get(), static_cast<btMultiBodyConstraintSolver *>(solver_.get()), collision_config_.get()));
    dynamics_world_->getDispatchInfo().m_useContinuous = true;

    dynamics_world_->setGravity(btVector3(0, -10, 0));

    dynamics_world_->setInternalTickCallback(tickCallback, static_cast<void *>(this)) ;

    broadphase_->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
}

PhysicsWorld::~PhysicsWorld()
{
}

btDynamicsWorld *PhysicsWorld::getDynamicsWorld() {
    return dynamics_world_.get();
}

void PhysicsWorld::setGravity(const Vector3f &g) {
    dynamics_world_->setGravity(toBulletVector(g)) ;
}

void PhysicsWorld::stepSimulation(float deltaTime) {
    if ( dynamics_world_ ) {
        dynamics_world_->stepSimulation(deltaTime, 10, 1. / 240.);
    }
}

class ContactResultCallback: public btCollisionWorld::ContactResultCallback {
public:

    ContactResultCallback(PhysicsWorld &w, std::vector<ContactResult> &contacts): results_(contacts), world_(w) {}

    btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) override {
        const btRigidBody *obA =  btRigidBody::upcast(colObj0Wrap->getCollisionObject()) ;
        const btRigidBody *obB =  btRigidBody::upcast(colObj1Wrap->getCollisionObject()) ;
        const btMultiBodyLinkCollider *colA = dynamic_cast<const btMultiBodyLinkCollider *>(colObj0Wrap->getCollisionObject());
        const btMultiBodyLinkCollider *colB = dynamic_cast<const btMultiBodyLinkCollider *>(colObj1Wrap->getCollisionObject());

        CollisionObject *pA = reinterpret_cast<CollisionObject *>(colObj0Wrap->getCollisionObject()->getUserPointer()) ;
        CollisionObject *pB = reinterpret_cast<CollisionObject *>(colObj1Wrap->getCollisionObject()->getUserPointer()) ;

        ContactResult result ;
        result.a_ = pA ;
        result.b_ = pB ;

        result.pa_ =  toEigenVector(cp.getPositionWorldOnA()) ;
        result.pb_ = toEigenVector(cp.getPositionWorldOnB()) ;
        result.normal_ = toEigenVector(cp.m_normalWorldOnB);

        results_.emplace_back(std::move(result)) ;


        return 0 ;
    }

    std::vector<ContactResult> &results_ ;
    PhysicsWorld &world_ ;


};

bool PhysicsWorld::contactTest(const RigidBodyPtr &b1, std::vector<ContactResult> &results) {
    ContactResultCallback cb(*this, results) ;
    dynamics_world_->contactTest(b1->handle(), cb) ;

    return results.size() ;
}

bool PhysicsWorld::rayPick(const Vector3f &origin, const Vector3f &dir, RayHitResult &res)
{
    btVector3 rayFromWorld = eigenVectorToBullet(origin) ;
    btVector3 rayToWorld = eigenVectorToBullet(dir*10000) ;

    btCollisionWorld::ClosestRayResultCallback rayCallback(rayFromWorld, rayToWorld);

    rayCallback.m_flags |= btTriangleRaycastCallback::kF_UseGjkConvexCastRaytest;
    dynamics_world_->rayTest(rayFromWorld, rayToWorld, rayCallback);

    if (rayCallback.hasHit()) {
        Vector3f pickPos = toEigenVector(rayCallback.m_hitPointWorld);
        Vector3f pickNorm = toEigenVector(rayCallback.m_hitNormalWorld) ;
        const btCollisionObject* body = rayCallback.m_collisionObject;

        res.p_ = pickPos ;
        res.n_ = pickNorm ;
        res.o_ = reinterpret_cast<CollisionObject *>(body->getUserPointer()) ;

        return true ;
    }

    return false ;


}


void PhysicsWorld::addCollisionShape(const btCollisionShape *shape) {
    collision_shapes_.push_back(shape) ;
}

uint PhysicsWorld::addBody(const RigidBodyPtr &body) {
    dynamics_world_->addRigidBody(body->handle());
    uint idx = bodies_.size() ;
    bodies_.emplace_back(body) ;
    body->handle()->setUserIndex(idx) ;
    body->handle()->setUserPointer(reinterpret_cast<void *>(body.get())) ;
    string bname = body->getName() ;
    if ( !bname.empty() )
        body_map_.emplace(bname, idx) ;
    return idx ;
}

uint PhysicsWorld::addMultiBody(const MultiBodyPtr &body) {
    body->create(*this) ;
    uint idx = multi_bodies_.size() ;
    multi_bodies_.emplace_back(body) ;
    body->handle()->setUserIndex(idx) ;
    body->handle()->setUserPointer(body.get());
    string bname = body->name() ;
    if ( !bname.empty() )
        multi_body_map_.emplace(bname, idx) ;
    return idx ;
}

uint PhysicsWorld::addGhost(const GhostObjectPtr &ghost)
{
     dynamics_world_->addCollisionObject(ghost->handle(), btBroadphaseProxy::SensorTrigger,btBroadphaseProxy::AllFilter & ~btBroadphaseProxy::SensorTrigger) ;

}

void PhysicsWorld::addConstraint(const Constraint &c) {
    dynamics_world_->addConstraint(c.handle());
    constraints_.emplace_back(std::move(c)) ;
}

RigidBodyPtr PhysicsWorld::getRigidBody(uint idx) const {
    return bodies_[idx];
}


MultiBodyPtr PhysicsWorld::getMultiBody(uint idx) const {
    return multi_bodies_[idx];
}

RigidBodyPtr PhysicsWorld::findRigidBody(const string &name) {
    auto it = body_map_.find(name) ;
    if ( it != body_map_.end() )
        return getRigidBody(it->second) ;
    else
        return nullptr ;
}

MultiBodyPtr PhysicsWorld::findMultiBody(const string &name) {
    auto it = multi_body_map_.find(name) ;
    if ( it != multi_body_map_.end() )
        return getMultiBody(it->second) ;
    else
        return nullptr ;
}

struct btCollisionFilter : public btOverlapFilterCallback
{
    CollisionFilter *filter_ = nullptr ;

    btCollisionFilter(CollisionFilter *f): filter_(f) {}

    // return true when pairs need collision
    virtual bool needBroadphaseCollision(btBroadphaseProxy *proxy0,  btBroadphaseProxy *proxy1) const  {
        assert(proxy0 != NULL && proxy1 != NULL) ;

        bool collide = (proxy0->m_collisionFilterGroup
                        & proxy1->m_collisionFilterMask) != 0;
        collide = collide && ( proxy1->m_collisionFilterGroup
                               & proxy0->m_collisionFilterMask);

        btCollisionObject *ob0 = static_cast<btCollisionObject *>(proxy0->m_clientObject);
        if ( !ob0 ) return collide ;

        btCollisionObject *ob1 = static_cast<btCollisionObject *>(proxy1->m_clientObject);
        if ( !ob1 ) return collide ;

        CollisionObject *co0 = static_cast<CollisionObject *>(ob0->getUserPointer());
        assert(co0 != nullptr) ;

        CollisionObject *co1 = static_cast<CollisionObject *>(ob1->getUserPointer());
        assert(co1 != nullptr) ;

        return filter_->collide(co0, co1) ;
    }
};

void PhysicsWorld::setCollisionFilter(CollisionFilter *f) {

    if ( f ) filter_callback_.reset(new btCollisionFilter(f));
    else filter_callback_.reset() ;

    btOverlappingPairCache* pair_cache = dynamics_world_->getPairCache();

    assert(pair_cache != nullptr ) ;

    pair_cache->setOverlapFilterCallback(filter_callback_.get());
}

void PhysicsWorld::addSensor(SensorPtr sensor) {
    assert(dynamics_world_);
    sensors_.push_back(sensor) ;
    sensor->init(*this) ;
}

void PhysicsWorld::setCollisionFeedback(CollisionFeedback *feedback) {
    collision_feedback_ = feedback ;
}

void PhysicsWorld::queryCollisions() {
    if  ( !collision_feedback_ ) return ;

    int numManifolds = dynamics_world_->getDispatcher()->getNumManifolds();
    for ( int i = 0; i < numManifolds; ++i ) {
        btPersistentManifold *contactManifold =
            dynamics_world_->getDispatcher()->getManifoldByIndexInternal(i);
        const btCollisionObject *obA = static_cast<const btCollisionObject *>(contactManifold->getBody0());
        const btCollisionObject *obB = static_cast<const btCollisionObject *>(contactManifold->getBody1());

        const CollisionObject *coA = static_cast<const CollisionObject *>(obA->getUserPointer());
        assert(coA != nullptr) ;

        const CollisionObject *coB = static_cast<const CollisionObject *>(obB->getUserPointer());
     //   assert(coB != nullptr) ;

        int numContacts = contactManifold->getNumContacts();

        for (int j = 0; j < numContacts; ++j) {
            btManifoldPoint &pt = contactManifold->getContactPoint(j);
            if (pt.getDistance() < 0.f)  {
                ContactResult result ;
                result.a_ = coA ;
                result.b_ = coB ;

                result.pa_ =  toEigenVector(pt.getPositionWorldOnA()) ;
                result.pb_ = toEigenVector(pt.getPositionWorldOnB()) ;
                result.normal_ = toEigenVector(pt.m_normalWorldOnB);

                collision_feedback_->processContact(result) ;
            }
        }
      }


}


UpdateSceneMotionState::UpdateSceneMotionState(const NodePtr &node): node_(node) {
    Affine3f world = node->globalTransform() ;
    world_trans_ = toBulletTransform(world) ;
    local_frame_.setIdentity() ;
}

UpdateSceneMotionState::UpdateSceneMotionState(const NodePtr &node, const btTransform &local): node_(node), local_frame_(local) {
    Affine3f world = node->globalTransform() ;
    world_trans_ = toBulletTransform(world) ;
}

void UpdateSceneMotionState::getWorldTransform(btTransform &centerOfMassWorldTrans) const {
    centerOfMassWorldTrans = world_trans_ ;
}

void UpdateSceneMotionState::setWorldTransform(const btTransform &centerOfMassWorldTrans) {
    world_trans_ = centerOfMassWorldTrans;
    Node *parent = node_->getParent() ;
    node_->matrix() = ( parent == nullptr ) ? toEigenTransform(world_trans_).matrix() : parent->globalTransform().matrix().inverse() * toEigenTransform(world_trans_).matrix() ;
}

bool RayPicker::movePickedBody(const Ray &ray)
{
    btVector3 rayFromWorld = eigenVectorToBullet(ray.getOrigin()) ;
    btVector3 rayToWorld = eigenVectorToBullet(ray.getDir()*10000) ;

    if (picked_body_ && picked_constraint_)
    {
        btPoint2PointConstraint* pickCon = static_cast<btPoint2PointConstraint*>(picked_constraint_);
        if (pickCon)
        {
            //keep it at the same picking distance

            btVector3 newPivotB;

            btVector3 dir = rayToWorld - rayFromWorld;
            dir.normalize();
            dir *= old_picking_dist_ ;;

            newPivotB = rayFromWorld + dir;
            pickCon->setPivotB(newPivotB);
            return true;
        }
    }

    if ( mb_picked_constraint_ ) {
        //keep it at the same picking distance

        btVector3 dir = rayToWorld - rayFromWorld;
        dir.normalize();
        dir *= old_picking_dist_ ;

        btVector3 newPivotB = rayFromWorld + dir;

        mb_picked_constraint_->setPivotInB(newPivotB);
    }

    return false;

}

bool RayPicker::pickBody(const Ray &ray)
{
    if (!world_) return false;

    btVector3 rayFromWorld = eigenVectorToBullet(ray.getOrigin()) ;
    btVector3 rayToWorld = eigenVectorToBullet(ray.getDir()*10000) ;

    btCollisionWorld::ClosestRayResultCallback rayCallback(rayFromWorld, rayToWorld);

    rayCallback.m_flags |= btTriangleRaycastCallback::kF_UseGjkConvexCastRaytest;
    world_->rayTest(rayFromWorld, rayToWorld, rayCallback);
    if (rayCallback.hasHit())
    {
        btVector3 pickPos = rayCallback.m_hitPointWorld;
        btRigidBody* body = (btRigidBody*)btRigidBody::upcast(rayCallback.m_collisionObject);
        if (body)
        {
            //other exclusions?
            if (!(body->isStaticObject() || body->isKinematicObject()))
            {
                picked_body_ = body;
                saved_state_ = picked_body_->getActivationState();
                picked_body_->setActivationState(DISABLE_DEACTIVATION);
                //printf("pickPos=%f,%f,%f\n",pickPos.getX(),pickPos.getY(),pickPos.getZ());
                btVector3 localPivot = body->getCenterOfMassTransform().inverse() * pickPos;
                btPoint2PointConstraint* p2p = new btPoint2PointConstraint(*body, localPivot);
                world_->addConstraint(p2p, true);
                picked_constraint_ = p2p;
                btScalar mousePickClamping = 30.f;
                p2p->m_setting.m_impulseClamp = mousePickClamping;
                //very weak constraint for picking
                p2p->m_setting.m_tau = 0.001f;
            }
        } else {
            btMultiBodyLinkCollider* multiCol = (btMultiBodyLinkCollider*)btMultiBodyLinkCollider::upcast(rayCallback.m_collisionObject);
            if (multiCol && multiCol->m_multiBody)
            {
                prev_can_sleep_ = multiCol->m_multiBody->getCanSleep();
                multiCol->m_multiBody->setCanSleep(false);

                btVector3 pivotInA = multiCol->m_multiBody->worldPosToLocal(multiCol->m_link, pickPos);

                btMultiBodyPoint2Point* p2p = new btMultiBodyPoint2Point(multiCol->m_multiBody, multiCol->m_link, 0, pivotInA, pickPos);
                //if you add too much energy to the system, causing high angular velocities, simulation 'explodes'
                //see also http://www.bulletphysics.org/Bullet/phpBB3/viewtopic.php?f=4&t=949
                //so we try to avoid it by clamping the maximum impulse (force) that the mouse pick can apply
                //it is not satisfying, hopefully we find a better solution (higher order integrator, using joint friction using a zero-velocity target motor with limited force etc?)
                btScalar scaling = 1;
                p2p->setMaxAppliedImpulse(2 * scaling);

                btMultiBodyDynamicsWorld* world = (btMultiBodyDynamicsWorld*)world_;
                world->addMultiBodyConstraint(p2p);

                mb_picked_constraint_ = p2p;

            }
        }

        //					pickObject(pickPos, rayCallback.m_collisionObject);
        old_picking_pos_ = rayToWorld;
        hit_pos_ = pickPos;
        old_picking_dist_ = (pickPos - rayFromWorld).length();
        //					printf("hit !\n");
        //add p2p
    }
    return false;
}

void RayPicker::removePickingConstraint()
{
    if (picked_constraint_)
    {
        picked_body_->forceActivationState(saved_state_);
        picked_body_->activate();
        world_->removeConstraint(picked_constraint_);
        delete picked_constraint_;
        picked_constraint_ = nullptr ;
        picked_body_ = nullptr;
    }

    if ( mb_picked_constraint_ ) {
        mb_picked_constraint_->getMultiBodyA()->setCanSleep(prev_can_sleep_);
        btMultiBodyDynamicsWorld* world = (btMultiBodyDynamicsWorld*)world_;
        world->removeMultiBodyConstraint(mb_picked_constraint_);
        delete mb_picked_constraint_ ;
        mb_picked_constraint_ = nullptr ;
    }

}


}}
