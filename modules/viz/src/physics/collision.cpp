#include <cvx/viz/physics/collision.hpp>

#include <bullet/BulletCollision/Gimpact/btGImpactShape.h>

using namespace std ;
using namespace Eigen ;

namespace cvx { namespace viz {

void TriangleMeshCollisionShape::create(const std::string &fname)
{
    const aiScene *sc = aiImportFile(fname.c_str(),
                                       aiProcess_PreTransformVertices
                                     | aiProcess_Triangulate
                                     | aiProcess_JoinIdenticalVertices
                                     | aiProcess_SortByPType
                                     | aiProcess_OptimizeMeshes
                                     ) ;

    if ( !sc ) return ;

    import(sc) ;
}

void TriangleMeshCollisionShape::import(const aiScene *scene) {
    importMeshes(scene) ;
}

void TriangleMeshCollisionShape::create(const aiScene *scene) {
    import(scene) ;
}



void TriangleMeshCollisionShape::importMeshes(const aiScene *scene) {
    indexed_vertex_array_.reset(new btTriangleIndexVertexArray);

    for(int i=0 ; i<scene->mNumMeshes ; i++) {
        aiMesh *mesh = scene->mMeshes[i] ;
        if ( mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE ) continue ;

        MeshData cmesh ;

        for( int j=0 ; j<mesh->mNumFaces ; j++ ) {
            aiFace &face = mesh->mFaces[j] ;
            cmesh.tridx_.push_back(face.mIndices[0]) ;
            cmesh.tridx_.push_back(face.mIndices[1]) ;
            cmesh.tridx_.push_back(face.mIndices[2]) ;
        }

        for( int j=0 ; j<mesh->mNumVertices ; j++ ) {
            aiVector3D &v = mesh->mVertices[j] ;
            cmesh.vtx_.emplace_back(v.x, v.y, v.z) ;
        }

        btIndexedMesh bulletMesh;

        bulletMesh.m_numTriangles = mesh->mNumFaces;

        bulletMesh.m_triangleIndexBase =
                reinterpret_cast<const unsigned char*>(cmesh.tridx_.data());
        bulletMesh.m_triangleIndexStride = 3 * sizeof(uint);
        bulletMesh.m_numVertices = mesh->mNumVertices ;
        bulletMesh.m_vertexBase =
                reinterpret_cast<const unsigned char*>(cmesh.vtx_.data());
        bulletMesh.m_vertexStride = sizeof(Eigen::Vector3f);
        bulletMesh.m_indexType = PHY_INTEGER;
        bulletMesh.m_vertexType = PHY_FLOAT;

        indexed_vertex_array_->addIndexedMesh(bulletMesh, PHY_INTEGER);  // exact shape
        meshes_.emplace_back(std::move(cmesh)) ;
    }

    handle_.reset(makeShape(indexed_vertex_array_.get())) ;
}


btCollisionShape *StaticMeshCollisionShape::makeShape(btTriangleIndexVertexArray *va) {
    return new btBvhTriangleMeshShape(va ,true);
}

// does not work
btCollisionShape *DynamicMeshCollisionShape::makeShape(btTriangleIndexVertexArray *va) {
    btGImpactMeshShape *shape = new btGImpactMeshShape(va) ;
    shape->updateBound();
    return shape ;
}

}}
