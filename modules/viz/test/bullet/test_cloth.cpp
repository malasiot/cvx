/* these should be included before QtGui */
/* there will be a warning that you cannot mix glew with Qt GL which you can ignore */

#include <cvx/viz/scene/material.hpp>


#include <QApplication>
#include <QMainWindow>

#include <cvx/util/misc/strings.hpp>
#include <cvx/util/math/rng.hpp>

#include <cvx/viz/scene/camera.hpp>
#include <cvx/viz/scene/light.hpp>

#include <cvx/viz/scene/node.hpp>
#include <cvx/viz/scene/geometry.hpp>
#include <cvx/viz/scene/mesh.hpp>

#include <iostream>

#include "physics.hpp"
#include "bullet_gui.hpp"

using namespace Eigen ;

using namespace std ;
using namespace cvx::viz ;
using namespace cvx::util ;


static Physics physics ;
static ScenePtr scene ;
static btSoftBody *cloth ;

class TestClothWidget: public TestAnimation {
public:
    TestClothWidget(ScenePtr scene, Physics &physics, btSoftBody *cloth): TestAnimation(scene, physics), cloth_(cloth) {
        node_ = scene_->findNodeByName("cloth") ;
    }

    void onAnimationUpdate() override {
        physics_.stepSimulation(1./60) ;
        updateClothGeometry() ;
    }

    void updateClothGeometry() {
        auto dr = node_->getDrawable(0) ;

        PointList3f vtx, colors ;

        for( uint i=0 ; i<cloth_->m_nodes.size() ; i++ ) {
            Vector3f v = toEigenVector(cloth_->m_nodes[i].m_x) ;
            vtx.push_back(v) ;
            colors.push_back({1, 0, 0}) ;
        }
        dr->setGeometry(make_shared<MeshGeometry>(Mesh::makePointCloud(vtx, colors))) ;
    }

    btSoftBody *cloth_ ;
    NodePtr node_ ;
};



NodePtr makeBox(const string &name, const Vector3f &hs, const Matrix4f &tr, const Vector4f &clr) {

    NodePtr box_node(new Node) ;
    box_node->setName(name) ;

    GeometryPtr geom(new BoxGeometry(hs)) ;

    MaterialInstancePtr material(new ConstantMaterialInstance(clr)) ;

    DrawablePtr dr(new Drawable(geom, material)) ;

    box_node->addDrawable(dr) ;

    box_node->matrix() = tr ;

    return box_node ;
}

NodePtr makeCylinder(const string &name, float radius, float length, const Matrix4f &tr, const Vector4f &clr) {

    // we need an extra node to perform rotation of cylinder so that it is aligned with Y axis instead of Z

    NodePtr node(new Node) ;
    node->setName(name) ;

    GeometryPtr geom(new CylinderGeometry(radius, length)) ;

    MaterialInstancePtr material(new ConstantMaterialInstance(clr)) ;

    DrawablePtr dr(new Drawable(geom, material)) ;

    node->addDrawable(dr) ;

    node->matrix().rotate(AngleAxisf(-0.5*M_PI, Vector3f::UnitX()));

    NodePtr externalNode(new Node) ;
    externalNode->matrix() = tr ;
    externalNode->addChild(node) ;

    return externalNode ;
}


btSoftBody *createSoftBody(const btScalar s, const int numX, const int numY,const int fixed)
{
    btSoftBody* cloth = btSoftBodyHelpers::CreatePatch(physics.softBodyWorldInfo,
                                                       btVector3(-s / 2, s - 1, 0),
                                                       btVector3(+s / 2, s - 1, 0),
                                                       btVector3(-s / 2, s - 1, +s),
                                                       btVector3(+s / 2, s - 1, +s),
                                                       numX, numY,
                                                       fixed, true);

    cloth->getCollisionShape()->setMargin(0.01f);
    cloth->getCollisionShape()->setUserPointer((void*)cloth);
    cloth->generateBendingConstraints(2, cloth->appendMaterial());
    cloth->setTotalMass(1);
  //  cloth->m_cfg.citerations = 100;
  //  cloth->m_cfg.diterations = 100;
    cloth->m_cfg.piterations = 5;
    cloth->m_cfg.kDP = 0.005f;

    btSoftRigidDynamicsWorld *w = reinterpret_cast<btSoftRigidDynamicsWorld *>(physics.getDynamicsWorld()) ;
    w->addSoftBody(cloth) ;

    NodePtr node(new Node) ;

    node->setName("cloth") ;
    MaterialInstancePtr material(new PerVertexColorMaterialInstance(0.5f)) ;

    DrawablePtr dr(new Drawable(nullptr, material)) ;

    node->addDrawable(dr) ;

    scene->addChild(node) ;

    return cloth ;

}

void createScene() {
    scene.reset(new Scene) ;

    // init physics

    physics.createSoftBodyDynamicsWorld();

    // add light
    std::shared_ptr<DirectionalLight> dl( new DirectionalLight(Vector3f(0.5, 0.5, 1)) ) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene->addLight(dl) ;

    // create ground plane object

    Affine3f tr ;
    tr.setIdentity() ;
    tr.translate(Vector3f(0, -50, 0)) ;

    NodePtr node = makeBox("ground", Vector3f{50., 50., 50.}, tr.matrix(), Vector4f{0.5, 0.5, 0.5, 1}) ;

    scene->addChild(node) ;

    /// create ground collision shape
    btCollisionShape *groundShape = physics.createBoxShape(btVector3(btScalar(50.), btScalar(50.), btScalar(50.)));

    btTransform groundTransform;
    groundTransform.setIdentity();
    groundTransform.setOrigin(btVector3(0, -50, 0));

    physics.createStaticRigidBody(groundTransform, groundShape);

    btCollisionShape *poleShape = physics.createBoxShape({1, 1, 1});

    btTransform poleTransform;
    poleTransform.setIdentity();
    poleTransform.setOrigin(btVector3(0, 1, 0));

    physics.createStaticRigidBody(poleTransform, poleShape);

    NodePtr pole_node = makeBox("pole", {1, 1, 1}, Affine3f(Translation3f(Vector3f{0, 1, 0})).matrix(), {0, 1, 0, 1}) ;
    scene->addChild(pole_node) ;


    const btScalar s = 4;  //size of cloth patch
    const int NUM_X = 91;  //vertices on X axis
    const int NUM_Z = 91;  //vertices on Z axis
    cloth = createSoftBody(s, NUM_X, NUM_Z, 1+2);

}


int main(int argc, char **argv)
{

    createScene() ;

    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setMajorVersion(3);
    format.setMinorVersion(3);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSwapInterval(1);

    format.setSamples(4);
    format.setProfile(QSurfaceFormat::CoreProfile);

    QSurfaceFormat::setDefaultFormat(format);

    QMainWindow window ;
    window.setCentralWidget(new TestClothWidget(scene, physics, cloth)) ;
    window.resize(512, 512) ;
    window.show() ;

    return app.exec();
}
