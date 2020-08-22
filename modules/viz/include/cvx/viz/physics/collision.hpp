#ifndef CVX_VIZ_PHYSICS_COLLISION_HPP
#define CVX_VIZ_PHYSICS_COLLISION_HPP

#include <memory>

#include <bullet/btBulletCollisionCommon.h>
#include <Eigen/Geometry>
#include <cvx/viz/physics/convert.hpp>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

namespace cvx { namespace viz {
class CollisionShape {

public:

    btCollisionShape *handle() const { return handle_.get() ; }

    Eigen::Vector3f computeLocalInertia(btScalar mass) {
        btVector3 inertia{0, 0, 0} ;
        handle_->calculateLocalInertia(mass, inertia) ;
        return toEigenVector(inertia) ;
    }

protected:

    friend class RigidBody ;

    std::shared_ptr<btCollisionShape> handle_ ;
};

class BoxCollisionShape: public CollisionShape {
public:
    BoxCollisionShape(const Eigen::Vector3f &hs) {
        handle_.reset(new btBoxShape(toBulletVector(hs)));
    }
};

class CylinderCollisionShape: public CollisionShape {
public:
    CylinderCollisionShape(float radius, float len) {
        handle_.reset(new btCylinderShape(btVector3(radius, len/2.0, radius)));
    }
};

class MeshCollisionShape: public CollisionShape {
public:
    MeshCollisionShape(const std::string &mesh) ;
    MeshCollisionShape(const aiScene *scene) ;

private:

    void create(const aiScene *scene) ;
};

} // namespace viz
} // namespace cvx

#endif
