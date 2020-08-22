#include <cvx/viz/physics/world.hpp>
#include <cvx/viz/physics/convert.hpp>
#include <bullet/BulletCollision/CollisionDispatch/btCollisionWorld.h>

using namespace Eigen ;

namespace cvx { namespace viz {

PhysicsWorld::PhysicsWorld() {}

void PhysicsWorld::createDefaultDynamicsWorld() {
    collision_config_.reset( new btDefaultCollisionConfiguration() ) ;
    //m_collisionConfiguration->setConvexConvexMultipointIterations();

    ///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
    dispatcher_.reset( new btCollisionDispatcher(collision_config_.get()) ) ;

    broadphase_.reset(new btDbvtBroadphase() ) ;

    ///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
    solver_.reset( new btSequentialImpulseConstraintSolver() ) ;

    dynamics_world_.reset( new btDiscreteDynamicsWorld(dispatcher_.get(), broadphase_.get(), solver_.get(), collision_config_.get()));

    dynamics_world_->setGravity(btVector3(0, -10, 0));
}

PhysicsWorld::~PhysicsWorld()
{
    //remove the rigidbodies from the dynamics world and delete them

    if ( dynamics_world_ ) {
        int i;
        for (i = dynamics_world_->getNumConstraints() - 1; i >= 0; i--) {
            dynamics_world_->removeConstraint(dynamics_world_->getConstraint(i));
        }
    }
}

btDynamicsWorld *PhysicsWorld::getDynamicsWorld() {
    return dynamics_world_.get();
}

void PhysicsWorld::stepSimulation(float deltaTime) {
    if ( dynamics_world_ ) {
        dynamics_world_->stepSimulation(deltaTime);
    }
}

class ContactResultCallback: public btCollisionWorld::ContactResultCallback {
public:

    ContactResultCallback(PhysicsWorld &w, std::vector<ContactResult> &contacts): results_(contacts), world_(w) {}

    btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) override {
        const btRigidBody *obA =  btRigidBody::upcast(colObj0Wrap->getCollisionObject()) ;
        const btRigidBody *obB =  btRigidBody::upcast(colObj1Wrap->getCollisionObject()) ;

        int idA = obA->getUserIndex() ;
        int idB = obB->getUserIndex() ;

        ContactResult result ;
        result.a_ = world_.findObjectById(idA) ;
        result.b_ = world_.findObjectById(idB) ;
        result.pa_ =  toEigenVector(cp.getPositionWorldOnA()) ;
        result.pb_ = toEigenVector(cp.getPositionWorldOnB()) ;
        result.normal_ = toEigenVector(cp.m_normalWorldOnB);

        results_.emplace_back(std::move(result)) ;

        return 0 ;
    }

    std::vector<ContactResult> &results_ ;
    PhysicsWorld &world_ ;


};

bool PhysicsWorld::contactTest(const RigidBody &b1, std::vector<ContactResult> &results) {
    ContactResultCallback cb(*this, results) ;
    dynamics_world_->contactTest(b1.handle(), cb) ;

    for( ContactResult &c: results ) {
        c.a_ = &b1 ;
    }
    return results.size() ;
}


void PhysicsWorld::addCollisionShape(const btCollisionShape *shape) {
    collision_shapes_.push_back(shape) ;
}

uint PhysicsWorld::addBody(const RigidBody &body) {
     dynamics_world_->addRigidBody(body.handle());
     bodies_.emplace(body_count_, body) ;
     body.handle()->setUserIndex(body_count_) ;
     return body_count_++ ;
}

void PhysicsWorld::addConstraint(const Constraint &c) {
    dynamics_world_->addConstraint(c.handle());
}

RigidBody *PhysicsWorld::findObjectById(int id)  {
    auto it = bodies_.find(id) ;
    if ( it == bodies_.end() ) return nullptr ;
    else return &(it->second) ;
}


UpdateSceneMotionState::UpdateSceneMotionState(const NodePtr &node): node_(node) {
    Affine3f world = node->globalTransform() ;
    world_trans_ = toBulletTransform(world) ;
}

void UpdateSceneMotionState::getWorldTransform(btTransform &centerOfMassWorldTrans) const {
    centerOfMassWorldTrans = world_trans_;
}

void UpdateSceneMotionState::setWorldTransform(const btTransform &centerOfMassWorldTrans)
{
    world_trans_ = centerOfMassWorldTrans;
    node_->matrix() = toEigenTransform(world_trans_);
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
        picked_constraint_ = 0;
        picked_body_ = 0;
    }

}


}}
