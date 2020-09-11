#pragma once

#include <Eigen/Geometry>
#include <cvx/viz/physics/world.hpp>
#include <cvx/viz/physics/collision.hpp>
#include <cvx/viz/physics/convert.hpp>
#include <cvx/viz/robot/urdf_robot.hpp>

#include <bullet/BulletDynamics/Featherstone/btMultiBodyJointMotor.h>

class MultiBody {
public:

    struct Joint ;

    struct Link {

        Link & setLocalInertialFrame(const Eigen::Isometry3f &f) {
            local_inertial_frame_ = cvx::viz::toBulletTransform(f);
            return *this ;
        }

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

    struct Joint {
        std::string name_ ;
        std:: string parent_, child_ ;
        JointType type_ ;
        btVector3 axis_ = {1, 0, 0};
        btTransform j2p_ ;
        float lower_, upper_, friction_, damping_, max_force_, max_velocity_ ;

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
    };

    struct Motor {
        std::string name_ ;
        btMultiBodyJointMotor* motor_ ;
        float target_velocity_ = 0.f;
        float max_impulse_ = 10.f ;

        void setTargetVelocity(float v) ;

    };

    Link &addLink(const std::string &name, float mass, cvx::viz::CollisionShape::Ptr cshape, const Eigen::Isometry3f &origin = Eigen::Isometry3f::Identity());

    Joint &addJoint(const std::string &name, JointType type, const std::string &parent, const std::string &child, const Eigen::Isometry3f &j2p);

    Motor *getMotor(const std::string &name) ;

    int findLink(const std::string &name);

    void buildTree();

    void buildCollisionObject(int link_idx, const btTransform &link_transform);

    void buildJoints(int link_idx, const btTransform &parent_transform_in_world_space);

    void create(cvx::viz::PhysicsWorld &physics);

    void createFromURDF(cvx::viz::PhysicsWorld &physics, cvx::viz::urdf::Robot &rb);


    void getLinkTransforms(std::map<std::string, Eigen::Isometry3f> &names) const ;

    std::vector<Link> links_ ;
    std::map<std::string, int> link_map_ ;
    std::map<std::string, Joint> joints_ ;
    std::unique_ptr<btMultiBody> body_ ;
    std::vector<std::unique_ptr<btMultiBodyConstraint>> constraints_ ;
    std::map<std::string, Motor> motors_ ;
    Link *root_ ;
};
