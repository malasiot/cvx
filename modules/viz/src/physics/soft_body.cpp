#include <cvx/viz/physics/soft_body.hpp>
#include <cvx/viz/physics/world.hpp>
#include <cvx/viz/physics/convert.hpp>
#include <cvx/viz/scene/mesh.hpp>

using namespace Eigen ;
using namespace std ;

namespace cvx { namespace viz {

SoftPatch2D::SoftPatch2D(PhysicsWorld &physics, const Vector3f &c00, const Vector3f &c10, const Vector3f &c01,
                         uint nvX, uint nvY, uint flags, bool gendiags) {
 #define IDX(_x_, _y_) ((_y_)*nvX + (_x_))
        /* Create nodes	*/
        assert( nvX >=2 && nvY >=2) ;
        uint numNodes = nvX * nvY ;

        unique_ptr<btVector3 []> x(new btVector3[numNodes]) ;
        unique_ptr<btScalar []> m(new btScalar[numNodes]) ;

        btScalar stepX = (c00 - c10).norm() / (btScalar)(nvX - 1) ;
        btScalar stepY = (c00 - c01).norm() / (btScalar)(nvY - 1) ;

        Vector3f deltaX = (c10 - c00).normalized() * stepX ;
        Vector3f deltaY = (c01 - c00).normalized() * stepY ;

        Vector3f py = c00 ;
        for ( uint iy = 0; iy < nvY; ++iy, py += deltaY ) {
            Vector3f px = py ;
            for ( uint ix = 0; ix < nvX; ++ix, px += deltaX ) {
                x[IDX(ix, iy)] = toBulletVector(px) ;
                m[IDX(ix, iy)] = 1;
            }
        }

        handle_.reset(new btSoftBody(&physics.getSoftBodyWorldInfo(), numNodes, x.get(), m.get())) ;

        if (flags & 1) handle_->setMass(IDX(0, 0), 0);
        if (flags & 2) handle_->setMass(IDX(nvX - 1, 0), 0);
        if (flags & 4) handle_->setMass(IDX(0, nvY - 1), 0);
        if (flags & 8) handle_->setMass(IDX(nvX - 1, nvY - 1), 0);

        /* Create links	and faces */
        for ( uint iy = 0; iy < nvY; ++iy) {
            for (uint ix = 0; ix < nvX; ++ix) {
                const uint idx = IDX(ix, iy);
                const bool mdx = (ix + 1) < nvX;
                const bool mdy = (iy + 1) < nvY;
                if (mdx) handle_->appendLink(idx, IDX(ix + 1, iy));
                if (mdy) handle_->appendLink(idx, IDX(ix, iy + 1));
                if (mdx && mdy) {
                    if ((ix + iy) & 1)
                    {
                        handle_->appendFace(IDX(ix, iy), IDX(ix + 1, iy), IDX(ix + 1, iy + 1));
                        handle_->appendFace(IDX(ix, iy), IDX(ix + 1, iy + 1), IDX(ix, iy + 1));
                        if (gendiags)
                        {
                            handle_->appendLink(IDX(ix, iy), IDX(ix + 1, iy + 1));
                        }
                    }
                    else
                    {
                        handle_->appendFace(IDX(ix, iy + 1), IDX(ix, iy), IDX(ix + 1, iy));
                        handle_->appendFace(IDX(ix, iy + 1), IDX(ix + 1, iy), IDX(ix + 1, iy + 1));
                        if (gendiags)
                        {
                            handle_->appendLink(IDX(ix + 1, iy), IDX(ix, iy + 1));
                        }
                    }
                }
            }
        }

       handle_->getCollisionShape()->setMargin(0.01f);
       handle_->getCollisionShape()->setUserPointer((void*)this);
       handle_->generateBendingConstraints(2, handle_->appendMaterial());
       handle_->setTotalMass(1);
     //  cloth->m_cfg.citerations = 100;
     //  cloth->m_cfg.diterations = 100;
       handle_->m_cfg.piterations = 5;
       handle_->m_cfg.kDP = 0.005f;

}

GeometryPtr SoftPatch2D::getVisualGeometry() const {

    vector<Vector3f> vertices ;
    vector<uint32_t> indices ;

    MeshPtr mesh(new Mesh(Mesh::Triangles)) ;

    std::map<btSoftBody::Node *, uint> idxs ;

    for( uint i=0 ; i<handle_->m_nodes.size() ; i++ ) {
        auto &node = handle_->m_nodes[i] ;
        Vector3f v = toEigenVector(node.m_x) ;
        mesh->vertices().data().push_back(v) ;
        mesh->normals().data().push_back(toEigenVector(node.m_n));
        idxs[&node] = i ;
    }

    for( uint i=0 ; i< handle_->m_faces.size() ; i++ ) {
        const auto &face = handle_->m_faces[i] ;
        uint idx0 = idxs[face.m_n[0]] ;
        uint idx1 = idxs[face.m_n[2]] ;
        uint idx2 = idxs[face.m_n[1]] ;
        mesh->vertices().indices().push_back(idx0) ;
        mesh->vertices().indices().push_back(idx1) ;
        mesh->vertices().indices().push_back(idx2) ;
    }


    return GeometryPtr(new MeshGeometry(mesh)) ;
}



}
}
