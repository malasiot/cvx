#pragma once

#include <Eigen/Geometry>
#include <cvx/viz/physics/world.hpp>
#include <cvx/viz/physics/collision.hpp>
#include <cvx/viz/physics/convert.hpp>
#include <cvx/viz/robot/urdf_robot.hpp>

#include <bullet/BulletDynamics/Featherstone/btMultiBodyJointMotor.h>

namespace cvx { namespace viz {


class Joint ;

class Link {

public:

    Link & setLocalInertialFrame(const Eigen::Isometry3f &f) {
        local_inertial_frame_ = toBulletTransform(f);
        return *this ;
    }

private:

    friend class MultiBody ;
    friend class Joint ;

    Link() {
        local_inertial_frame_.setIdentity() ;
    }

    float mass_ ;
    std::string name_ ;
    Link *parent_link_ = nullptr ;
    Joint *parent_joint_ = nullptr ;
    std::vector<Link *> child_links_ ;
    std::vector<Joint *> child_joints_ ;
    cvx::viz::CollisionShape::Ptr shape_ ;
    std::unique_ptr<btCollisionShape> proxy_ ;
    std::unique_ptr<btMultiBodyLinkCollider> collider_ ;

    btVector3 inertia_ ;
    Eigen::Isometry3f origin_ ;
    btTransform local_inertial_frame_ ;
    int mb_index_ ;
};


enum JointType { RevoluteJoint, ContinuousJoint, PrismaticJoint, FixedJoint, SphericalJoint, FloatingJoint, PlanarJoint } ;

class Joint {
public:

    Link *parentLink() const { return parent_link_ ; }
    Link *childLink() const { return child_link_ ; }

    Joint& setAxis(const Eigen::Vector3f &axis) {
         axis_ = cvx::viz::toBulletVector(axis) ;
         return *this ;
    }

    Joint& setLimits(float l, float u) {
         lower_ = l ;
         upper_ = u ;
         return *this ;
    }

    Joint& setFriction(float f) {
        friction_ = f ;
        return *this ;
    }

    Joint& setDamping(float d) {
        damping_ = d ;
        return *this ;
    }

    Joint& setMaxVelocity(float v) {
        max_velocity_ = v ;
        return *this ;
    }

    Joint& setMaxForce(float v) {
        max_force_ = v ;
        return *this ;
    }

    Joint & setMotorMaxImpulse(float v) ;

    void setMimicJointPosition() ;

    float getPosition() ;
    float getVelocity() ;
    float getTorque() ;

    void setPosition(float v) ;
    void setVelocity(float v) ;
    void setTorque(float v) ;

    void setTargetVelocity(float v);
    void setTargetPosition(float v);


private:

    friend class MultiBody ;

    Joint() = default ;

    std::string name_ ;
    std:: string parent_, child_ ;
    Link *parent_link_ = nullptr, *child_link_ = nullptr ;

    JointType type_ ;
    btVector3 axis_ = {1, 0, 0};
    btTransform j2p_ ;
    float lower_ = 0.f, upper_ = 0.f, friction_ = 0.f, damping_ = 0.f, max_force_ = 0.f, max_velocity_ = 0.f ;
    btMultiBody *body_ = nullptr ;
    btMultiBodyJointMotor* motor_ = nullptr ;
    btScalar motor_max_force_ = btScalar(1000.0) ;
    std::vector<Joint *> mimic_joints_ ; // child joints to be mimicked
    float mimic_multiplier_ = 1.f, mimic_offset_ = 0.f ;
};

class MultiBody {
public:
    Link &addLink(const std::string &name, float mass, cvx::viz::CollisionShape::Ptr cshape, const Eigen::Isometry3f &origin = Eigen::Isometry3f::Identity());

    Joint &addJoint(const std::string &name, JointType type, const std::string &parent, const std::string &child, const Eigen::Isometry3f &j2p);

    float getJointPosition(const std::string &name) ;
    float getJointVelocity(const std::string &name) ;
    float getJointTorque(const std::string &name) ;

    void setJointPosition(const std::string &name, float v) ;
    void setJointVelocity(const std::string &name, float v) ;
    void setJointTorque(const std::string &name, float v) ;

    void setTargetVelocity(const std::string &name, float v) ;
    void setTargetPosition(const std::string &name, float v) ;

    void setName(const std::string &name) ;
    std::string name() const ;

    Joint *findJoint(const std::string &name) ;

    void create(cvx::viz::PhysicsWorld &physics, const Eigen::Isometry3f &root_tr = Eigen::Isometry3f::Identity());

    void loadURDF(cvx::viz::urdf::Robot &rb);

    void getLinkTransforms(std::map<std::string, Eigen::Isometry3f> &names) const ;

private:

    void buildTree();

    void buildCollisionObject(int link_idx, const btTransform &link_transform);

    void buildJoints(int link_idx, const btTransform &parent_transform_in_world_space);

    int findLink(const std::string &name);

    int getJointIndex(const std::string &name);

    void setMimic(const urdf::Joint &joint, Joint &j);

private:
    std::vector<Link> links_ ;
    std::map<std::string, int> link_map_ ;
    std::map<std::string, Joint> joints_ ;
    std::unique_ptr<btMultiBody> body_ ;
    std::vector<std::unique_ptr<btMultiBodyConstraint>> constraints_ ;
    std::string name_ ;

    Link *root_ ;


};

} // namespace viz
} // namespace cvx
