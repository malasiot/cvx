#include <cvx/viz/physics/rigid_body.hpp>

namespace cvx { namespace viz {

RigidBody::RigidBody(btScalar mass, btMotionState *ms, const CollisionShapePtr &shape, const Eigen::Vector3f &localInertia) {
    btAssert((!shape.handle() || shape.handle()->getShapeType() != INVALID_SHAPE_PROXYTYPE));

    collision_shape_ = shape ;
    motion_state_.reset(ms) ;

    btVector3 inertia = eigenVectorToBullet(localInertia) ;
    btRigidBody::btRigidBodyConstructionInfo cInfo(mass, ms, shape->handle(), inertia);

    btRigidBody* body = new btRigidBody(cInfo);

    handle_.reset(body);
}

// dynamic body with intertia computed by collision shape and mass
RigidBody::RigidBody(btScalar mass, btMotionState *ms, const CollisionShapePtr &shape) {
    btAssert((!shape->handle() || shape->handle_()->getShapeType() != INVALID_SHAPE_PROXYTYPE));

    collision_shape_ = shape ;
    motion_state_.reset(ms) ;

    btVector3 localInertia(0, 0, 0);

    shape->handle()->calculateLocalInertia(mass, localInertia);
    btRigidBody::btRigidBodyConstructionInfo cInfo(mass, ms, shape->handle(), localInertia);

    btRigidBody* body = new btRigidBody(cInfo);

    handle_.reset(body) ;
}

// static body
RigidBody::RigidBody(const CollisionShapePtr &shape, const Eigen::Affine3f &tr) {
    btVector3 localInertia(0, 0, 0);

    collision_shape_ = shape ;

    btDefaultMotionState* myMotionState = new btDefaultMotionState(toBulletTransform(tr));

    motion_state_.reset(myMotionState) ;

    btRigidBody::btRigidBodyConstructionInfo cInfo(btScalar(0.), myMotionState, shape->handle(), localInertia);

    btRigidBody* body = new btRigidBody(cInfo);

    handle_.reset(body) ;
}

btRigidBody *RigidBody::handle() const { return handle_.get() ; }

void RigidBody::setName(const std::string &name) { name_ = name ; }

Eigen::Isometry3f RigidBody::getWorldTransform() const {
    if ( motion_state_) {
        btTransform world ;
        motion_state_->getWorldTransform(world) ;
        return toEigenTransform(world) ;
    } else {
        return toEigenTransform(handle_->getWorldTransform()) ;
    }

}


} // namespace viz
} // namespace cvx
