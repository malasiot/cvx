#include <cvx/viz/physics/soft_body.hpp>
#include <cvx/viz/physics/world.hpp>
#include <cvx/viz/physics/convert.hpp>

using namespace Eigen ;
namespace cvx { namespace viz {

SoftPatch2D::SoftPatch2D(PhysicsWorld &physics, float s, uint numX, uint numY, uint flags) {
    btSoftBody* cloth = btSoftBodyHelpers::CreatePatch(
                physics.getSoftBodyWorldInfo(),
                                                          btVector3(-s / 2, s - 1, 0),
                                                          btVector3(+s / 2, s - 1, 0),
                                                          btVector3(-s / 2, s - 1, +s),
                                                          btVector3(+s / 2, s - 1, +s),
                                                          numX, numY,
                                                          flags, true);

       cloth->getCollisionShape()->setMargin(0.01f);
       cloth->getCollisionShape()->setUserPointer((void*)this);
       cloth->generateBendingConstraints(2, cloth->appendMaterial());
       cloth->setTotalMass(1);
     //  cloth->m_cfg.citerations = 100;
     //  cloth->m_cfg.diterations = 100;
       cloth->m_cfg.piterations = 5;
       cloth->m_cfg.kDP = 0.005f;

       handle_.reset(cloth) ;
}

util::PointList3f SoftBody::getVertices() const {
    util::PointList3f vtx ;
    for( uint i=0 ; i<handle_->m_nodes.size() ; i++ ) {
        Vector3f v = toEigenVector(handle_->m_nodes[i].m_x) ;
        vtx.push_back(v) ;
    }
    return vtx ;
}

}
}
