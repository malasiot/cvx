#ifndef BULLET_PHYSICS_HPP
#define BULLET_PHYSICS_HPP

#include <bullet/btBulletCollisionCommon.h>
#include <bullet/btBulletDynamicsCommon.h>

#include <memory>
#include <vector>

#include <cvx/viz/scene/node.hpp>

struct Physics
{


    std::vector<btCollisionShape*> m_collisionShapes;
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

    btDiscreteDynamicsWorld* getDynamicsWorld()
    {
        return m_dynamicsWorld.get();
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
        //delete collision shapes
        for (int j = 0; j < m_collisionShapes.size(); j++) {
            btCollisionShape* shape = m_collisionShapes[j];
            delete shape;
        }

        m_collisionShapes.clear();

        m_dynamicsWorld.release() ;
        m_solver.release();
        m_broadphase.release();
        m_dispatcher.release();
        m_collisionConfiguration.release() ;

    }

    void debugDraw(int debugDrawFlags)
    {
        if (m_dynamicsWorld)
        {
            if (m_dynamicsWorld->getDebugDrawer())
            {
                m_dynamicsWorld->getDebugDrawer()->setDebugMode(debugDrawFlags);
            }
            m_dynamicsWorld->debugDrawWorld();
        }
    }


    btBoxShape* createBoxShape(const btVector3& halfExtents)
    {
        btBoxShape* box = new btBoxShape(halfExtents);
        return box;
    }

    void deleteRigidBody(btRigidBody* body)
    {
        int graphicsUid = body->getUserIndex();
 //       m_guiHelper->removeGraphicsInstance(graphicsUid);

        m_dynamicsWorld->removeRigidBody(body);
        btMotionState* ms = body->getMotionState();
        delete body;
        delete ms;
    }

    btRigidBody* createRigidBody(float mass, const btTransform& startTransform, btCollisionShape* shape, const btVector4& color = btVector4(1, 0, 0, 1))
    {
        btAssert((!shape || shape->getShapeType() != INVALID_SHAPE_PROXYTYPE));

        //rigidbody is dynamic if and only if mass is non zero, otherwise static
        bool isDynamic = (mass != 0.f);

        btVector3 localInertia(0, 0, 0);
        if (isDynamic)
            shape->calculateLocalInertia(mass, localInertia);

        //using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects

#define USE_MOTIONSTATE 1
#ifdef USE_MOTIONSTATE
        btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);

        btRigidBody::btRigidBodyConstructionInfo cInfo(mass, myMotionState, shape, localInertia);

        btRigidBody* body = new btRigidBody(cInfo);
        //body->setContactProcessingThreshold(m_defaultContactProcessingThreshold);

#else
        btRigidBody* body = new btRigidBody(mass, 0, shape, localInertia);
        body->setWorldTransform(startTransform);
#endif  //

        body->setUserIndex(-1);
        m_dynamicsWorld->addRigidBody(body);
        return body;
    }


    void addTransformObserver(btRigidBody *body, TransformObserver o) {
        observers_.insert(make_pair(body, o)) ;
    }

    static inline Eigen::Vector3f toEigenVector(const std::vector<float>& vec) {return Eigen::Vector3f(vec[0],vec[1],vec[2]);}
    static inline Eigen::Vector3f toEigenVector(const btVector3& vec) {return Eigen::Vector3f(vec.x(),vec.y(),vec.z());}

    static Eigen::Affine3f toEigenTransform(const btTransform& transform) {
      btVector3 transBullet = transform.getOrigin();
      btQuaternion quatBullet = transform.getRotation();
      Eigen::Translation3f transEig;
      transEig = Eigen::Translation3f(toEigenVector(transBullet));
      Eigen::Matrix3f rotEig = Eigen::Quaternionf(quatBullet.w(),quatBullet.x(),quatBullet.y(),quatBullet.z()).toRotationMatrix();
      Eigen::Affine3f out(transEig*rotEig);
      return out;
    }
    void updateTransforms() {
        for ( auto &lp: observers_ ) {
            btRigidBody *rb = lp.first ;
            TransformObserver ob = lp.second ;

            btTransform trans ;
            rb->getMotionState()->getWorldTransform(trans);

            Eigen::Affine3f etr = toEigenTransform(trans) ;

            ob(etr) ;
        }
    }
};









#endif
