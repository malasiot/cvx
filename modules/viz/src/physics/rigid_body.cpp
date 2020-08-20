#include <cvx/viz/physics/rigid_body.hpp>

namespace cvx { namespace viz {

RigidBody::RigidBody(btScalar mass, btMotionState *ms, const CollisionShape &shape, const Eigen::Vector3f &localInertia) {
    btAssert((!shape.handle_ || shape.handle_->getShapeType() != INVALID_SHAPE_PROXYTYPE));

    btVector3 inertia = eigenVectorToBullet(localInertia) ;
    btRigidBody::btRigidBodyConstructionInfo cInfo(mass, ms, shape.handle(), inertia);

    handle_ = new btRigidBody(cInfo);
}

// dynamic body with intertia computed by collision shape and mass
RigidBody::RigidBody(btScalar mass, btMotionState *ms, const CollisionShape &shape) {
    btAssert((!shape.handle_ || shape.handle_->getShapeType() != INVALID_SHAPE_PROXYTYPE));
    btVector3 localInertia(0, 0, 0);

    shape.handle()->calculateLocalInertia(mass, localInertia);
    btRigidBody::btRigidBodyConstructionInfo cInfo(mass, ms, shape.handle(), localInertia);

    handle_ = new btRigidBody(cInfo);
}

// static body
RigidBody::RigidBody(const CollisionShape &shape, const Eigen::Affine3f &tr) {
    btVector3 localInertia(0, 0, 0);

    btDefaultMotionState* myMotionState = new btDefaultMotionState(toBulletTransform(tr));

    btRigidBody::btRigidBodyConstructionInfo cInfo(btScalar(0.), myMotionState, shape.handle(), localInertia);

    btRigidBody* body = new btRigidBody(cInfo);

    body->setUserIndex(-1);

    handle_ = body ;
}




} // namespace viz
} // namespace cvx
