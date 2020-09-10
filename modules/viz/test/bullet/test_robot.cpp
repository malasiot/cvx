#include <cvx/viz/renderer/renderer.hpp>
#include <cvx/viz/scene/camera.hpp>
#include <cvx/viz/scene/light.hpp>
#include <cvx/viz/scene/node.hpp>
#include <cvx/viz/scene/marker.hpp>
#include <cvx/viz/scene/material.hpp>
#include <cvx/viz/scene/geometry.hpp>

#include <cvx/viz/robot/robot_scene.hpp>

#include <cvx/util/math/rng.hpp>
#include <cvx/util/misc/format.hpp>

#include <cvx/viz/physics/kinematic.hpp>

#include <iostream>
#include <thread>
#include <cvx/viz/physics/world.hpp>

#include "bullet_gui.hpp"
#include "multi_body.hpp"

#include <QApplication>
#include <QMainWindow>

using namespace cvx::viz ;
using namespace cvx::util ;

using namespace std ;
using namespace Eigen ;

PhysicsWorld physics ;
ScenePtr scene(new Scene) ;



MultiBody body ;


class GUI: public TestSimulation {
public:
    GUI(cvx::viz::ScenePtr scene, cvx::viz::PhysicsWorld &physics):
    TestSimulation(scene, physics) {
    }

    void onUpdate(float delta) override {

        btTransform tr_base = body.links_[0].collider_->getWorldTransform() ;
         btTransform tr_link1 = body.links_[1].collider_->getWorldTransform() ;
         btTransform tr_link2 = body.links_[2].collider_->getWorldTransform() ;
         btTransform tr_link3 = body.links_[3].collider_->getWorldTransform() ;
         cout << toEigenTransform(tr_link3).matrix() << endl ;
         scene->findNodeByName("base")->matrix() = toEigenTransform(tr_base) ;
         scene->findNodeByName("link1")->matrix() = toEigenTransform(tr_link1) ;
         scene->findNodeByName("link2")->matrix() = toEigenTransform(tr_link2) ;
         scene->findNodeByName("link3")->matrix() = toEigenTransform(tr_link3) ;

    TestSimulation::onUpdate(delta) ;
    }

};

NodePtr makeCube(const string &name, const Vector3f &hs, const Vector4f &clr, NodePtr parent) {
    PhongMaterialInstance *mat = new PhongMaterialInstance() ;
    mat->setDiffuse(clr) ;

    auto node = parent->addSimpleShapeNode(GeometryPtr(new BoxGeometry(hs)), MaterialInstancePtr(mat)) ;
    node->setName(name) ;
    return node ;
}

NodePtr makeSphere(const string &name, float radious, const Vector4f &clr, NodePtr parent) {
    PhongMaterialInstance *mat = new PhongMaterialInstance() ;
    mat->setDiffuse(clr) ;

    auto node = parent->addSimpleShapeNode(GeometryPtr(new SphereGeometry(radious)), MaterialInstancePtr(mat)) ;
    node->setName(name) ;
    return node ;
}

NodePtr makeCylinder(const string &name, float radious, float len, const Vector4f &clr, NodePtr parent) {
    PhongMaterialInstance *mat = new PhongMaterialInstance() ;
    mat->setDiffuse(clr) ;

    auto node = parent->addSimpleShapeNode(GeometryPtr(new CylinderGeometry(radious, len)), MaterialInstancePtr(mat)) ;
    node->setName(name) ;
    return node ;
}

NodePtr makeJoint(const Isometry3f &tr, NodePtr parent) {
    NodePtr j(new Node) ;
    j->matrix() = tr ;
    parent->addChild(j) ;
    return j ;
}


void createScene() {

    physics.createMultiBodyDynamicsWorld();

    Affine3f tr(Translation3f{0, -1.5, 0}) ;

    Vector3f ground_hs{3.5f, 0.05f, 3.5f} ;
    scene->addBox(ground_hs, tr.matrix(), Vector4f{0.5, 0.5, 0.5, 1}) ;
    physics.addBody(RigidBody(CollisionShape::Ptr(new BoxCollisionShape(ground_hs)), tr)) ;

    DirectionalLight *dl = new DirectionalLight(Vector3f(0.5, 0.5, 1)) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene->addLight(LightPtr(dl)) ;



    float link_size = 0.2 ;
    Vector3f box_hs{0.05, link_size/2, 0.05} ;
    float box_mass = 0.1 ;

    CollisionShape::Ptr box_shape(new BoxCollisionShape(box_hs)) ;

    Isometry3f offset ;
    offset.setIdentity() ;
    offset.translate(Vector3f{0, -link_size/2, 0}) ;

    body.addLink("base", box_mass, box_shape, offset).setLocalInertialFrame(offset) ;
    body.addLink("link1", box_mass, box_shape, offset).setLocalInertialFrame(offset) ;
    body.addLink("link2", box_mass, box_shape, offset).setLocalInertialFrame(offset) ;
    body.addLink("link3", box_mass, box_shape, offset).setLocalInertialFrame(offset) ;

    Vector3f axis = {1, 0, 0} ;
    Isometry3f j2p ;
    j2p.setIdentity() ;
    j2p.translate(Vector3f{0, -link_size, 0}) ;
    body.addJoint("j1", MultiBody::RevoluteJoint, "base", "link1", j2p).setAxis(axis) ;
    body.addJoint("j2", MultiBody::RevoluteJoint, "link1", "link2", j2p).setAxis(axis) ;
    body.addJoint("j3", MultiBody::RevoluteJoint, "link2", "link3", j2p).setAxis(axis) ;

    body.create(physics) ;



    Isometry3f base_tr ;
    base_tr.setIdentity() ;
/*
    NodePtr base_node = makeCube(box_hs, {1, 0, 0, 1}, scene);
    NodePtr j1_node = makeJoint(j2p, base_node) ;
    NodePtr link1_node = makeCube(box_hs, {1, 1, 0, 1}, j1_node) ;
    NodePtr j2_node = makeJoint(j2p, link1_node) ;
    NodePtr link2_node = makeCube(box_hs, {1, 0, 1, 1}, j2_node) ;
    NodePtr j3_node = makeJoint(j2p, link2_node) ;
    NodePtr link3_node = makeCube(box_hs, {0, 0, 1, 1}, j3_node) ;
*/
    NodePtr base_node = makeCube("base", box_hs, {1, 0, 0, 1}, scene);

    NodePtr link1_node = makeCube("link1", box_hs, {1, 1, 0, 1}, scene) ;

    NodePtr link2_node = makeCube("link2", box_hs, {1, 0, 1, 1}, scene) ;

    NodePtr link3_node = makeCube("link3", box_hs, {0, 0, 1, 1}, scene) ;


}





int main(int argc, char **argv)
{
    createScene() ;

    QApplication app(argc, argv);

    cvx::viz::SimpleQtViewer::initDefaultGLContext() ;

    QMainWindow window ;
    window.setCentralWidget(new GUI(scene, physics)) ;
    window.resize(512, 512) ;
    window.show() ;

    return app.exec();
}
