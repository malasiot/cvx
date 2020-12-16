/* these should be included before QtGui */
/* there will be a warning that you cannot mix glew with Qt GL which you can ignore */

#include <cvx/viz/scene/material.hpp>
#include <cvx/util/geometry/point_list.hpp>

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

#include <cvx/viz/physics/world.hpp>
#include <cvx/viz/physics/soft_body.hpp>

#include "bullet_gui.hpp"

using namespace Eigen ;

using namespace std ;
using namespace cvx::viz ;
using namespace cvx::util ;


static PhysicsWorld physics ;
static ScenePtr scene ;
static btSoftBody *cloth ;

static GeometryPtr makeClothGeometry(const PointList3f &vtx) {
    PointList3f  colors ;

    for( uint i=0 ; i<vtx.size() ; i++ ) {
       colors.push_back({1, 0, 0}) ;
    }
    return make_shared<MeshGeometry>(Mesh::makePointCloud(vtx, colors)) ;
}

class TestClothWidget: public SimulationGui {
public:
    TestClothWidget(ScenePtr scene, PhysicsWorld &physics, const Vector3f &c, float r): SimulationGui(scene, physics, c, r) {
        node_ = scene_->findNodeByName("cloth") ;
        cloth_ = physics.findSoftBody("cloth");
    }

    void onUpdate(float delta) override {
        SimulationGui::onUpdate(delta) ;
        updateClothGeometry() ;
    }

    void updateClothGeometry() {
        auto dr = node_->getDrawable(0) ;

        dr->setGeometry(makeClothGeometry(cloth_->getVertices())) ;
    }

    SoftBodyPtr cloth_ ;
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

void createScene() {
    scene.reset(new Scene) ;

    // init physics

    physics.createSoftBodyDynamicsWorld();

    // add light
    std::shared_ptr<DirectionalLight> dl( new DirectionalLight(Vector3f(0.5, 0.5, 1)) ) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene->addLight(dl) ;

    // create ground plane object

    Affine3f tr(Translation3f{0, -50, 0}) ;

    Vector3f ground_hs{50., 50., 50.} ;
    scene->addBox(ground_hs, tr.matrix(), {0.5, 0.5, 0.5, 1})->setName("ground") ;
    RigidBodyPtr ground = make_shared<RigidBody>(CollisionShapePtr(new BoxCollisionShape(ground_hs)), tr);
    ground->setName("ground") ;
    physics.addRigidBody(ground) ;

    // create static pole
    Affine3f poleTransform ;
    poleTransform.setIdentity() ;
    poleTransform.translate(Vector3f{.15, 2.5, 0.5}) ;
    poleTransform.rotate(AngleAxisf(0.5*M_PI, Vector3f::UnitZ()));
      poleTransform.translate(Vector3f{-0.15, -2.5, 0.5}) ;
    scene->addCylinder(0.25, 10, poleTransform.matrix(), {0, 1, 0, 1})->setName("pole") ;
    RigidBodyPtr pole = make_shared<RigidBody>(CollisionShapePtr(new CylinderCollisionShape(0.25, 10)), poleTransform);
    pole->setName("pole") ;
    physics.addRigidBody(pole) ;


    const btScalar s = 4;  //size of cloth patch
    const int NUM_X = 91;  //vertices on X axis
    const int NUM_Z = 91;  //vertices on Z axis

    SoftBodyPtr sb(new SoftPatch2D(physics, s, NUM_X, NUM_Z, 1+2));
    sb->setName("cloth");
    physics.addSoftBody(sb) ;

    PointList3f vertices = sb->getVertices() ;

    NodePtr node(new Node) ;

    node->setName("cloth") ;
    MaterialInstancePtr material(new PerVertexColorMaterialInstance(0.5f)) ;

    DrawablePtr dr(new Drawable(makeClothGeometry(vertices), material)) ;

    node->addDrawable(dr) ;

    scene->addChild(node) ;



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
    window.setCentralWidget(new TestClothWidget(scene, physics, { 0, 0, 0}, 13.0)) ;
    window.resize(512, 512) ;
    window.show() ;

    return app.exec();
}
