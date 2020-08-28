#ifndef CVX_VIZ_PHYSICS_KINEMATIC_HPP
#define CVX_VIZ_PHYSICS_KINEMATIC_HPP

#include <memory>

#include <bullet/btBulletCollisionCommon.h>
#include <bullet/btBulletDynamicsCommon.h>

#include <cvx/viz/physics/collision.hpp>
#include <cvx/viz/physics/convert.hpp>
#include <cvx/viz/robot/urdf_robot.hpp>
#include <cvx/viz/physics/rigid_body.hpp>

namespace cvx { namespace viz {

class ArticulatedCollisionShape: public GroupCollisionShape {
public :

    ArticulatedCollisionShape(const urdf::Robot &robot) {
        create(robot) ;
    }

private:

    struct Link {
        uint col_shape_index_ ;
        Eigen::Isometry3f origin_ ;
    };

    struct Joint {
        Eigen::Isometry3f origin_ ;
        Eigen::Vector3f axis_ ;
        float lower_limit_, upper_limit_ ;
        std::string type_, mimic_ ;
        Link *parent_, *child_ ;
        float position_ ;
    };

    std::map<std::string, Link> links_ ;
    std::map<std::string, Joint> joints_ ;

    void create(const urdf::Robot &robot) ;
    static CollisionShape::Ptr makeCollisionShape(const urdf::Geometry *geom) ;
};

} // namespace viz
} // namespace cvx

#endif
