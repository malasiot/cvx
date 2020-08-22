#include <cvx/viz/physics/collision.hpp>

#include <map>

using namespace std ;

namespace cvx { namespace viz {

MeshCollisionShape::MeshCollisionShape(const std::string &fname)
{
    const aiScene *sc = aiImportFile(fname.c_str(),
    aiProcess_PreTransformVertices
  | aiProcess_Triangulate
  | aiProcess_JoinIdenticalVertices
  | aiProcess_SortByPType
  | aiProcess_OptimizeMeshes
  ) ;

    if ( !sc ) return ;

    create(sc) ;
}

MeshCollisionShape::MeshCollisionShape(const aiScene *scene) {
    handle_.reset(new btCompoundShape()) ;
    create(scene) ;
}

void MeshCollisionShape::create(const aiScene *scene) {
    for(int i=0 ; i<scene->mNumMeshes ; i++) {
        aiMesh *mesh = scene->mMeshes[i] ;
        if ( mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE ) continue ;

        std::map<aiFace*, uint> face_map ;
        vector<uint> tridx ;

        for( int j=0 ; j<mesh->mNumFaces ; j++ ) {
            aiFace &face = mesh->mFaces[j] ;
            tridx.push_back(face.mIndices[0]) ;
            tridx.push_back(face.mIndices[1]) ;
            tridx.push_back(face.mIndices[2]) ;
        }

        btIndexedMesh bulletMesh;

        bulletMesh.m_numTriangles = mesh->mNumFaces;

         bulletMesh.m_triangleIndexBase =
               reinterpret_cast<const unsigned char*>(tridx.data());
          bulletMesh.m_triangleIndexStride = 3 * sizeof(uint);
         bulletMesh.m_numVertices = mesh->mNumVertices ;
           bulletMesh.m_vertexBase =
               reinterpret_cast<const unsigned char*>(mesh->mVertices);
           bulletMesh.m_vertexStride = sizeof(aiVector3D);
           bulletMesh.m_indexType = PHY_INTEGER;
           bulletMesh.m_vertexType = PHY_FLOAT;
           std::unique_ptr<btTriangleIndexVertexArray> indexedVertexArray(new btTriangleIndexVertexArray);
           indexedVertexArray->addIndexedMesh(bulletMesh, PHY_INTEGER);  // exact shape

           //! Embed 3D mesh into bullet shape
           //! btBvhTriangleMeshShape is the most generic/slow choice
           //! which allows concavity if the object is static
           btBvhTriangleMeshShape *meshShape = new btBvhTriangleMeshShape(indexedVertexArray.get(),true);

           btTransform tr ;
           tr.setIdentity() ;
           static_cast<btCompoundShape *>(handle_.get())->addChildShape(tr, meshShape) ;
    }
}
}}
