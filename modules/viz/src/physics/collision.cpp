#include <cvx/viz/physics/collision.hpp>

#include <map>

using namespace std ;

namespace cvx { namespace viz {

MeshCollisionShape::MeshCollisionShape(const std::string &fname, const Eigen::Affine3f &tr)
{
    const aiScene *sc = aiImportFile(fname.c_str(),
                                     aiProcess_PreTransformVertices
                                     | aiProcess_Triangulate
                                     | aiProcess_JoinIdenticalVertices
                                     | aiProcess_SortByPType
                                     | aiProcess_OptimizeMeshes
                                     ) ;

    if ( !sc ) return ;

    handle_.reset(new btCompoundShape()) ;
    create(sc, tr) ;
}

MeshCollisionShape::MeshCollisionShape(const aiScene *scene, const Eigen::Affine3f &tr) {
    handle_.reset(new btCompoundShape()) ;
    create(scene,  tr) ;
}

void MeshCollisionShape::create(const aiScene *scene, const Eigen::Affine3f &tr) {
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
        cmesh.indexed_vertex_array_.reset(new btTriangleIndexVertexArray);
        cmesh.indexed_vertex_array_->addIndexedMesh(bulletMesh, PHY_INTEGER);  // exact shape

        //! Embed 3D mesh into bullet shape
        //! btBvhTriangleMeshShape is the most generic/slow choice
        //! which allows concavity if the object is static
        cmesh.mesh_shape_.reset(new btBvhTriangleMeshShape(cmesh.indexed_vertex_array_.get(),true));

        btTransform bt = toBulletTransform(tr) ;
        static_cast<btCompoundShape *>(handle_.get())->addChildShape(bt, cmesh.mesh_shape_.get()) ;

        meshes_.emplace_back(std::move(cmesh)) ;
    }
}
}}
