#ifndef CVX_VIZ_PHYSICS_COLLISION_HPP
#define CVX_VIZ_PHYSICS_COLLISION_HPP

#include <cvx/viz/scene/node.hpp>

#include <memory>

#include <bullet/btBulletCollisionCommon.h>

namespace cvx { namespace viz {
class CollisionShape {

public:
    btCollisionShape *handle() const { return handle_ ; }

private:

    friend class PhysicsWorld ;

    CollisionShape(btCollisionShape *cs): handle_(cs) {}

    btCollisionShape *handle_ ;
};

} // namespace viz
} // namespace cvx

#endif
