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
    enum Axis { XAxis, YAxis, ZAxis } ;
    CylinderCollisionShape(float radius, float len, Axis a = YAxis ) {
        switch ( a ) {
        case XAxis:
            handle_.reset(new btCylinderShapeX(btVector3(radius, len/2.0, radius)));
            break ;
        case ZAxis:
            handle_.reset(new btCylinderShapeZ(btVector3(radius, len/2.0, radius)));
            break ;
        default:
            handle_.reset(new btCylinderShape(btVector3(radius, len/2.0, radius)));
        }
    }
};


class SphereCollisionShape: public CollisionShape {
public:
    SphereCollisionShape(float radius) {
        handle_.reset(new btSphereShape(radius));
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

class GroupCollisionShape: public CollisionShape {
public:
    GroupCollisionShape() ;

    void addChild(CollisionShape::Ptr c, const Eigen::Affine3f &tr) ;

private:

    std::vector<CollisionShape::Ptr> children_ ;
};


class CollisionObject {

public:
    virtual std::string getName() const = 0 ;
    virtual ~CollisionObject() {}
};

} // namespace viz
} // namespace cvx

#endif
