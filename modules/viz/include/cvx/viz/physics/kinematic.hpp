#ifndef CVX_VIZ_PHYSICS_KINEMATIC_HPP
#define CVX_VIZ_PHYSICS_KINEMATIC_HPP

#include <memory>

#include <bullet/btBulletCollisionCommon.h>
#include <bullet/btBulletDynamicsCommon.h>

#include <cvx/viz/physics/collision.hpp>
#include <cvx/viz/physics/convert.hpp>
#include <cvx/viz/robot/urdf_robot.hpp>

namespace cvx { namespace viz {

class KinematicBody {
public :
    // dynamic body with given inertia
    KinematicBody(const urdf::Robot &robot) ;

private:

    friend class PhysicsWorld ;

};

} // namespace viz
} // namespace cvx

#endif
