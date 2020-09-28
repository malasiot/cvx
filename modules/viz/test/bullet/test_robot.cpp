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
#include <cvx/viz/physics/multi_body.hpp>

#include "bullet_gui.hpp"

#include <QApplication>
#include <QMainWindow>

using namespace cvx::viz ;
using namespace cvx::util ;

using namespace std ;
using namespace Eigen ;

PhysicsWorld physics ;
ScenePtr scene(new Scene) ;



MultiBodyPtr body(new MultiBody) ;



class GUI: public SimulationGui {
public:
    GUI(cvx::viz::ScenePtr scene, cvx::viz::PhysicsWorld &physics):
    SimulationGui(scene, physics) {
    }

    void onUpdate(float delta) override {
         map<string, Isometry3f> transforms ;
        body->getLinkTransforms(transforms) ;
        scene->updateTransforms(transforms) ;
    SimulationGui::onUpdate(delta) ;

    RigidBodyPtr b = physics_.getRigidBody(1) ;

    vector<ContactResult> contacts ;
    if ( physics_.contactTest(b, contacts ) ) {
        for( ContactResult &c: contacts ) {

                cout << c.a_->getName() << ' ' << c.b_->getName() << endl ;
        }
    }
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
    RigidBodyPtr ground_rb(new RigidBody(CollisionShape::Ptr(new BoxCollisionShape(ground_hs)), tr));
    ground_rb->setName("ground");
    physics.addBody(ground_rb) ;

    DirectionalLight *dl = new DirectionalLight(Vector3f(0.5, 0.5, 1)) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene->addLight(LightPtr(dl)) ;

    float link_size = 0.4 ;
    Vector3f box_hs{0.05, link_size/2, 0.05} ;
    float box_mass = 0.1 ;

    CollisionShape::Ptr box_shape(new BoxCollisionShape(box_hs)) ;

    Isometry3f offset ;
    offset.setIdentity() ;
    offset.translate(Vector3f{0, -link_size/2, 0}) ;

    body->addLink("base", 0.0, box_shape, offset).setLocalInertialFrame(offset) ;
    body->addLink("link1", box_mass, box_shape, offset).setLocalInertialFrame(offset) ;
    body->addLink("link2", box_mass, box_shape, offset).setLocalInertialFrame(offset) ;
    body->addLink("link3", box_mass, box_shape, offset).setLocalInertialFrame(offset) ;

    Vector3f axis = {1, 0, 0} ;
    Isometry3f j2p ;
    j2p.setIdentity() ;
    j2p.translate(Vector3f{0, -link_size, 0}) ;
    auto &j1 = body->addJoint("j1", RevoluteJoint, "base", "link1", j2p).setAxis(axis) ;
    auto &j2 = body->addJoint("j2", RevoluteJoint, "link1", "link2", j2p).setAxis(axis) ;
    auto &j3 = body->addJoint("j3", RevoluteJoint, "link2", "link3", j2p).setAxis(axis) ;

    physics.addMultiBody(body) ;

    j1.setMotorMaxImpulse(0) ;
    j2.setMotorMaxImpulse(0) ;
    j3.setMotorMaxImpulse(0) ;

    NodePtr base_node = makeCube("base", box_hs, {1, 0, 0, 1}, scene);
    NodePtr link1_node = makeCube("link1", box_hs, {1, 1, 0, 1}, scene) ;
    NodePtr link2_node = makeCube("link2", box_hs, {1, 0, 1, 1}, scene) ;
    NodePtr link3_node = makeCube("link3", box_hs, {0, 0, 1, 1}, scene) ;

    Vector3f col_hs{0.1f, 0.1f, 0.1f} ;
    Isometry3f col_tr = Isometry3f::Identity();
    col_tr.translate(Vector3f{0.0, -1.5+0.2, -0.25});
    NodePtr cube = makeCube("cube", col_hs, {1, 0 , 0, 1}, scene) ;
    cube->matrix() = col_tr ;
    RigidBodyPtr cube_rb(new RigidBody(0.15, new UpdateSceneMotionState(cube), CollisionShape::Ptr(new BoxCollisionShape(col_hs)))) ;
    cube_rb->setName("cube") ;
    physics.addBody(cube_rb) ;
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
