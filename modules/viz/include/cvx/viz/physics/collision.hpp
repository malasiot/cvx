#ifndef CVX_VIZ_PHYSICS_COLLISION_HPP
#define CVX_VIZ_PHYSICS_COLLISION_HPP

#include <memory>
#include <map>

#include <bullet/btBulletCollisionCommon.h>
#include <Eigen/Geometry>
#include <cvx/viz/physics/convert.hpp>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <bullet/BulletCollision/Gimpact/btGImpactShape.h>

namespace cvx { namespace viz {
class CollisionShape {

public:
    using Ptr = std::shared_ptr<CollisionShape> ;

    btCollisionShape *handle() const { return handle_.get() ; }

    Eigen::Vector3f computeLocalInertia(btScalar mass) {
        btVector3 inertia{0, 0, 0} ;
        handle_->calculateLocalInertia(mass, inertia) ;
        return toEigenVector(inertia) ;
    }

    void setLocalScale(float scale) {
        handle_->setLocalScaling(btVector3(scale, scale, scale)) ;
    }

protected:

    friend class RigidBody ;

    std::unique_ptr<btCollisionShape> handle_ ;
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

class TriangleMeshCollisionShape: public CollisionShape {
protected:

    void importMeshes(const aiScene *scene) ;

    struct MeshData {
        std::vector<uint> tridx_ ;
        std::vector<Eigen::Vector3f> vtx_ ;
    } ;

    virtual void create(const aiScene *scene) ;
    virtual void create(const std::string &fname) ;

    virtual void import(const aiScene *scene) ;

    std::vector<MeshData> meshes_ ;
    std::unique_ptr<btTriangleIndexVertexArray> indexed_vertex_array_ ;

    virtual btCollisionShape *makeShape(btTriangleIndexVertexArray *va)  = 0 ;
};

class StaticMeshCollisionShape: public TriangleMeshCollisionShape {
public:
    StaticMeshCollisionShape(const std::string &mesh) { create(mesh) ; } ;

    // use aiProcess_PreTransformVertices and  aiProcess_Triangulate to prepare the geometries
    StaticMeshCollisionShape(const aiScene *scene) { create(scene) ; }

private:

    virtual btCollisionShape *makeShape(btTriangleIndexVertexArray *va) override ;

};

class DynamicMeshCollisionShape: public TriangleMeshCollisionShape {
public:
    DynamicMeshCollisionShape(const std::string &mesh) { create(mesh) ; } ;
    DynamicMeshCollisionShape(const aiScene *scene) { create(scene) ; }

private:

    virtual btCollisionShape *makeShape(btTriangleIndexVertexArray *va) override ;

};

} // namespace viz
} // namespace cvx

#endif
