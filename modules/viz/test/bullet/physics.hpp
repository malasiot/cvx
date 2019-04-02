#ifndef BULLET_PHYSICS_HPP
#define BULLET_PHYSICS_HPP

#include <bullet/btBulletCollisionCommon.h>
#include <bullet/btBulletDynamicsCommon.h>
#include <bullet/BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>

#include <memory>
#include <vector>

#include <cvx/viz/scene/node.hpp>

using namespace Eigen ;

inline Eigen::Vector3f toEigenVector(const std::vector<float>& vec) {return Eigen::Vector3f(vec[0],vec[1],vec[2]);}
inline Eigen::Vector3f toEigenVector(const btVector3& vec) {return Eigen::Vector3f(vec.x(),vec.y(),vec.z());}
inline btVector3 toBulletVector(const Eigen::Vector3f& vec) {return btVector3(vec[0],vec[1],vec[2]);}

inline Eigen::Affine3f toEigenTransform(const btTransform& transform) {
  btVector3 transBullet = transform.getOrigin();
  btQuaternion quatBullet = transform.getRotation();
  Eigen::Translation3f transEig;
  transEig = Eigen::Translation3f(toEigenVector(transBullet));
  Eigen::Matrix3f rotEig = Eigen::Quaternionf(quatBullet.w(),quatBullet.x(),quatBullet.y(),quatBullet.z()).toRotationMatrix();
  Eigen::Affine3f out(transEig*rotEig);
  return out;
}

inline btTransform toBulletTransform(const Eigen::Affine3f& affine) {
  Eigen::Vector3f transEig = affine.translation();
  Eigen::Matrix3f rotEig = affine.rotation();
  Eigen::Quaternionf quatEig = Eigen::Quaternionf(rotEig);
  btVector3 transBullet = toBulletVector(transEig);
  btQuaternion quatBullet = btQuaternion(quatEig.x(), quatEig.y(), quatEig.z(), quatEig.w());
  return btTransform(quatBullet,transBullet);
}

class MotionState: public btMotionState {
    public:

    MotionState ( const cvx::viz::NodePtr &node): node_(node) {
        Affine3f world = node->globalTransform() ;
        world_trans_ = toBulletTransform(world) ;

    }

    virtual void getWorldTransform( btTransform& centerOfMassWorldTrans ) const override {
        centerOfMassWorldTrans = world_trans_;
    }

    /// synchronizes world transform from physics to user
    /// Bullet only calls the update of worldtransform for active objects
    virtual void setWorldTransform( const btTransform& centerOfMassWorldTrans )
    {
        world_trans_ = centerOfMassWorldTrans;
        node_->matrix() = toEigenTransform(world_trans_);
    }

private:

    btTransform	world_trans_;
    cvx::viz::NodePtr node_ ;
};

struct Physics
{


    std::vector<std::shared_ptr<btCollisionShape>> collision_shapes_ ;
    std::unique_ptr<btBroadphaseInterface> m_broadphase ;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher ;
    std::unique_ptr<btConstraintSolver> m_solver ;
    std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfiguration ;
    std::unique_ptr<btDiscreteDynamicsWorld> m_dynamicsWorld;

    //data for picking objects
    class btRigidBody* m_pickedBody = nullptr;
    class btTypedConstraint* m_pickedConstraint = nullptr;
    int m_savedState;
    btVector3 m_oldPickingPos;
    btVector3 m_hitPos;
    btScalar m_oldPickingDist;


    using TransformObserver = std::function<void(const Eigen::Affine3f &)> ;
    std::map<btRigidBody *, TransformObserver> observers_ ;


    static btVector3 eigenVectorToBullet(const Eigen::Vector3f &v) {
        return btVector3(btScalar(v.x()), btScalar(v.y()), btScalar(v.z())) ;
    }
    btDiscreteDynamicsWorld* getDynamicsWorld()
    {
        return m_dynamicsWorld.get();
    }

    void addCollisionShape(const std::shared_ptr<btCollisionShape> &shape) {
        collision_shapes_.push_back(shape) ;
    }
    virtual void createEmptyDynamicsWorld()
    {
        ///collision configuration contains default setup for memory, collision setup
        m_collisionConfiguration.reset( new btDefaultCollisionConfiguration() ) ;
        //m_collisionConfiguration->setConvexConvexMultipointIterations();

        ///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
        m_dispatcher.reset( new btCollisionDispatcher(m_collisionConfiguration.get()) ) ;

        m_broadphase.reset(new btDbvtBroadphase() ) ;

        ///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
        m_solver.reset( new btSequentialImpulseConstraintSolver() ) ;

        m_dynamicsWorld.reset( new btDiscreteDynamicsWorld(m_dispatcher.get(), m_broadphase.get(), m_solver.get(), m_collisionConfiguration.get()));

        m_dynamicsWorld->setGravity(btVector3(0, -10, 0));
    }

    void stepSimulation(float deltaTime)
    {
        if ( m_dynamicsWorld ) {
            m_dynamicsWorld->stepSimulation(deltaTime);
        }
    }

    void physicsDebugDraw(int debugFlags)
    {
        if (m_dynamicsWorld && m_dynamicsWorld->getDebugDrawer())
        {
            m_dynamicsWorld->getDebugDrawer()->setDebugMode(debugFlags);
            m_dynamicsWorld->debugDrawWorld();
        }
    }

    void exitPhysics()
    {
     //   removePickingConstraint();

        //remove the rigidbodies from the dynamics world and delete them

        if ( m_dynamicsWorld )
        {
            int i;
            for (i = m_dynamicsWorld->getNumConstraints() - 1; i >= 0; i--)
            {
                m_dynamicsWorld->removeConstraint(m_dynamicsWorld->getConstraint(i));
            }
            for (i = m_dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
            {
                btCollisionObject* obj = m_dynamicsWorld->getCollisionObjectArray()[i];
                btRigidBody* body = btRigidBody::upcast(obj);
                if (body && body->getMotionState())
                {
                    delete body->getMotionState();
                }
                m_dynamicsWorld->removeCollisionObject(obj);
                delete obj;
            }
        }

        m_dynamicsWorld.release() ;
        m_solver.release();
        m_broadphase.release();
        m_dispatcher.release();
        m_collisionConfiguration.release() ;

    }


    bool movePickedBody(const btVector3& rayFromWorld, const btVector3& rayToWorld)
    {
        if (m_pickedBody && m_pickedConstraint)
        {
            btPoint2PointConstraint* pickCon = static_cast<btPoint2PointConstraint*>(m_pickedConstraint);
            if (pickCon)
            {
                //keep it at the same picking distance

                btVector3 newPivotB;

                btVector3 dir = rayToWorld - rayFromWorld;
                dir.normalize();
                dir *= m_oldPickingDist;

                newPivotB = rayFromWorld + dir;
                pickCon->setPivotB(newPivotB);
                return true;
            }
        }
        return false;
    }

    bool pickBody(const btVector3& rayFromWorld, const btVector3& rayToWorld)
    {
        if (!m_dynamicsWorld) return false;

        btCollisionWorld::ClosestRayResultCallback rayCallback(rayFromWorld, rayToWorld);

        rayCallback.m_flags |= btTriangleRaycastCallback::kF_UseGjkConvexCastRaytest;
        m_dynamicsWorld->rayTest(rayFromWorld, rayToWorld, rayCallback);
        if (rayCallback.hasHit())
        {
            btVector3 pickPos = rayCallback.m_hitPointWorld;
            btRigidBody* body = (btRigidBody*)btRigidBody::upcast(rayCallback.m_collisionObject);
            if (body)
            {
                //other exclusions?
                if (!(body->isStaticObject() || body->isKinematicObject()))
                {
                    m_pickedBody = body;
                    m_savedState = m_pickedBody->getActivationState();
                    m_pickedBody->setActivationState(DISABLE_DEACTIVATION);
                    //printf("pickPos=%f,%f,%f\n",pickPos.getX(),pickPos.getY(),pickPos.getZ());
                    btVector3 localPivot = body->getCenterOfMassTransform().inverse() * pickPos;
                    btPoint2PointConstraint* p2p = new btPoint2PointConstraint(*body, localPivot);
                    m_dynamicsWorld->addConstraint(p2p, true);
                    m_pickedConstraint = p2p;
                    btScalar mousePickClamping = 30.f;
                    p2p->m_setting.m_impulseClamp = mousePickClamping;
                    //very weak constraint for picking
                    p2p->m_setting.m_tau = 0.001f;
                }
            }

            //					pickObject(pickPos, rayCallback.m_collisionObject);
            m_oldPickingPos = rayToWorld;
            m_hitPos = pickPos;
            m_oldPickingDist = (pickPos - rayFromWorld).length();
            //					printf("hit !\n");
            //add p2p
        }
        return false;
    }

    virtual void removePickingConstraint()
    {
        if (m_pickedConstraint)
        {
            m_pickedBody->forceActivationState(m_savedState);
            m_pickedBody->activate();
            m_dynamicsWorld->removeConstraint(m_pickedConstraint);
            delete m_pickedConstraint;
            m_pickedConstraint = 0;
            m_pickedBody = 0;
        }
    }

    std::shared_ptr<btCollisionShape> createBoxShape(const btVector3& halfExtents)  {
        auto shape = std::make_shared<btBoxShape>(halfExtents);
        addCollisionShape(shape);
        return shape ;
    }

    void deleteRigidBody(btRigidBody* body)
    {
        m_dynamicsWorld->removeRigidBody(body);
        btMotionState* ms = body->getMotionState();
        delete body;
        delete ms;
    }

    btRigidBody* createRigidBody(float mass, const cvx::viz::NodePtr &node, btCollisionShape* shape, const btVector3 &localInertia)
    {
        btAssert((!shape || shape->getShapeType() != INVALID_SHAPE_PROXYTYPE));

        MotionState* myMotionState = new MotionState(node);

        btRigidBody::btRigidBodyConstructionInfo cInfo(mass, myMotionState, shape, localInertia);

        btRigidBody* body = new btRigidBody(cInfo);
        //body->setContactProcessingThreshold(m_defaultContactProcessingThreshold);

        m_dynamicsWorld->addRigidBody(body);
        return body;
    }

    btRigidBody* createStaticRigidBody(const btTransform &startTransform, btCollisionShape* shape)
    {
        btAssert((!shape || shape->getShapeType() != INVALID_SHAPE_PROXYTYPE));

        btVector3 localInertia(0, 0, 0);

        btRigidBody* body = new btRigidBody(btScalar(0.), nullptr, shape, localInertia);
        body->setWorldTransform(startTransform);

        body->setUserIndex(-1);
        m_dynamicsWorld->addRigidBody(body);
        return body;
    }

};









#endif
